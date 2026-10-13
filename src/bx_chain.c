#include "include/bx_chain.h"
#include "include/bx_utils.h"

#include <limits.h>
#include <pthread.h>
#include <string.h>

static pthread_mutex_t chain_mu = PTHREAD_MUTEX_INITIALIZER;
static BXChainCursor cursor = {.step = BX_CHAIN_CONTACT, .offset = 0, .pass = 0};
static bool contact_boot = true;
static bool project_boot = true;
/* True only after this process listed invoices from offset 0. A pass loaded
 * from the database did not see those rows in this cache. */
static bool invoice_listed_here = false;
/* List from offset 0 finished in this process. Kept when the pass-1 row
 * fails to store, so the retry does not list again or skip missed ids. */
static bool invoice_list_done = false;
static bool invoice_missed_done = false;

static bx_chain_load_fn store_load = NULL;
static bx_chain_save_fn store_save = NULL;
static void *store_ctx = NULL;

static bx_chain_contact_pass_fn walk_contact = NULL;
static bx_chain_list_fn walk_project = NULL;
static bx_chain_list_fn walk_invoice = NULL;
static bx_chain_missed_fn walk_missed = NULL;

typedef enum {
  BX_CHAIN_SKIP = 0,
  BX_CHAIN_OWN,
  BX_CHAIN_PLAIN
} BXChainAction;

typedef struct {
  MYSQL *conn;
  BXChainStep step;
  int pass;
} PageCtx;

const char *bx_chain_step_name(BXChainStep step) {
  switch (step) {
  case BX_CHAIN_PROJECT:
    return "project";
  case BX_CHAIN_INVOICE:
    return "invoice";
  case BX_CHAIN_CONTACT:
  default:
    return "contact";
  }
}

bool bx_chain_parse_step(const char *name, BXChainStep *step) {
  if (name == NULL || step == NULL) {
    return false;
  }
  if (strcmp(name, "contact") == 0) {
    *step = BX_CHAIN_CONTACT;
    return true;
  }
  if (strcmp(name, "project") == 0) {
    *step = BX_CHAIN_PROJECT;
    return true;
  }
  if (strcmp(name, "invoice") == 0) {
    *step = BX_CHAIN_INVOICE;
    return true;
  }
  return false;
}

void bx_chain_set_store(bx_chain_load_fn load, bx_chain_save_fn save, void *ctx) {
  store_load = load;
  store_save = save;
  store_ctx = ctx;
}

void bx_chain_set_walks(bx_chain_contact_pass_fn contact_pass,
                        bx_chain_list_fn project, bx_chain_list_fn invoice,
                        bx_chain_missed_fn invoice_missed) {
  walk_contact = contact_pass;
  walk_project = project;
  walk_invoice = invoice;
  walk_missed = invoice_missed;
}

static void normalize(BXChainCursor *c) {
  if (c->step != BX_CHAIN_CONTACT && c->step != BX_CHAIN_PROJECT &&
      c->step != BX_CHAIN_INVOICE) {
    c->step = BX_CHAIN_CONTACT;
  }
  if (c->offset < 0) {
    c->offset = 0;
  }
  if (c->step == BX_CHAIN_PROJECT) {
    c->pass = 0;
  }
  if (c->pass != 0 && c->pass != 1) {
    c->pass = 0;
  }
}

void bx_chain_init(void) {
  BXChainCursor loaded = {.step = BX_CHAIN_CONTACT, .offset = 0, .pass = 0};
  if (store_load != NULL) {
    if (!store_load(store_ctx, &loaded)) {
      loaded = (BXChainCursor){.step = BX_CHAIN_CONTACT, .offset = 0, .pass = 0};
    }
  }
  normalize(&loaded);
  pthread_mutex_lock(&chain_mu);
  cursor = loaded;
  contact_boot = true;
  project_boot = true;
  invoice_listed_here = false;
  invoice_list_done = false;
  invoice_missed_done = false;
  pthread_mutex_unlock(&chain_mu);
  bx_log_info("Sync chain %s next offset %d pass %d",
              bx_chain_step_name(loaded.step), loaded.offset, loaded.pass);
}

void bx_chain_read(BXChainCursor *out) {
  if (out == NULL) {
    return;
  }
  pthread_mutex_lock(&chain_mu);
  *out = cursor;
  pthread_mutex_unlock(&chain_mu);
}

static BXillError save_cursor(MYSQL *conn, BXChainStep step, int pass,
                              int offset) {
  BXChainCursor next = {.step = step, .pass = pass, .offset = offset};
  normalize(&next);
  if (store_save == NULL || !store_save(store_ctx, conn, &next)) {
    return ErrorGeneric;
  }
  cursor = next;
  return NoError;
}

static BXillError on_validated_page(void *v, int64_t page_offset) {
  PageCtx *page = v;
  int next_off = 0;

  if (page == NULL || page_offset < 0 ||
      page_offset > (int64_t)INT_MAX - BXILL_LIST_LIMIT) {
    return ErrorGeneric;
  }
  next_off = (int)page_offset + BXILL_LIST_LIMIT;
  pthread_mutex_lock(&chain_mu);
  if (cursor.step != page->step) {
    pthread_mutex_unlock(&chain_mu);
    return ErrorGeneric;
  }
  BXillError e = save_cursor(page->conn, page->step, page->pass, next_off);
  pthread_mutex_unlock(&chain_mu);
  return e;
}

static BXChainAction begin_contact(int *pass, int *offset) {
  pthread_mutex_lock(&chain_mu);
  if (contact_boot) {
    contact_boot = false;
    if (cursor.step != BX_CHAIN_CONTACT) {
      pthread_mutex_unlock(&chain_mu);
      return BX_CHAIN_SKIP;
    }
  }
  if (cursor.step == BX_CHAIN_CONTACT) {
    *pass = cursor.pass;
    *offset = cursor.offset;
    pthread_mutex_unlock(&chain_mu);
    return BX_CHAIN_OWN;
  }
  pthread_mutex_unlock(&chain_mu);
  return BX_CHAIN_PLAIN;
}

static BXChainAction begin_project(int *offset) {
  pthread_mutex_lock(&chain_mu);
  if (project_boot) {
    project_boot = false;
    if (cursor.step == BX_CHAIN_INVOICE) {
      pthread_mutex_unlock(&chain_mu);
      return BX_CHAIN_SKIP;
    }
  }
  if (cursor.step == BX_CHAIN_PROJECT) {
    *offset = cursor.offset;
    pthread_mutex_unlock(&chain_mu);
    return BX_CHAIN_OWN;
  }
  pthread_mutex_unlock(&chain_mu);
  return BX_CHAIN_PLAIN;
}

static BXChainAction begin_invoice(int *offset, int *pass) {
  pthread_mutex_lock(&chain_mu);
  if (cursor.step == BX_CHAIN_INVOICE) {
    *offset = cursor.offset;
    *pass = cursor.pass;
    pthread_mutex_unlock(&chain_mu);
    return BX_CHAIN_OWN;
  }
  pthread_mutex_unlock(&chain_mu);
  return BX_CHAIN_PLAIN;
}

static BXChainResult plain_contact(bXill *app, MYSQL *conn, Cache *cache) {
  BXChainResult r = {.error = NoError, .allow_prune = false};
  if (walk_contact == NULL) {
    r.error = ErrorGeneric;
    return r;
  }
  r.error = walk_contact(app, conn, cache, false, 0, NULL, NULL);
  if (r.error != NoError) {
    return r;
  }
  r.error = walk_contact(app, conn, cache, true, 0, NULL, NULL);
  r.allow_prune = (r.error == NoError);
  return r;
}

BXChainResult bx_chain_sync_contact(bXill *app, MYSQL *conn, Cache *cache) {
  BXChainResult r = {.error = NoError, .allow_prune = false};
  int pass = 0;
  int offset = 0;
  bool from_start = false;
  PageCtx live = {.conn = conn, .step = BX_CHAIN_CONTACT, .pass = 0};
  PageCtx archived = {.conn = conn, .step = BX_CHAIN_CONTACT, .pass = 1};
  BXChainAction action = begin_contact(&pass, &offset);

  if (action == BX_CHAIN_SKIP) {
    bx_log_info("Sync chain skips contacts; resume is past this step");
    return r;
  }
  if (action == BX_CHAIN_PLAIN) {
    return plain_contact(app, conn, cache);
  }
  if (walk_contact == NULL) {
    r.error = ErrorGeneric;
    return r;
  }
  from_start = (pass == 0 && offset == 0);
  if (pass <= 0) {
    r.error = walk_contact(app, conn, cache, false, offset, on_validated_page,
                           &live);
    if (r.error != NoError) {
      return r;
    }
    pthread_mutex_lock(&chain_mu);
    r.error = save_cursor(conn, BX_CHAIN_CONTACT, 1, 0);
    pthread_mutex_unlock(&chain_mu);
    if (r.error != NoError) {
      return r;
    }
    r.error = walk_contact(app, conn, cache, true, 0, on_validated_page,
                           &archived);
  } else {
    r.error = walk_contact(app, conn, cache, true, offset, on_validated_page,
                           &archived);
  }
  if (r.error != NoError) {
    return r;
  }
  pthread_mutex_lock(&chain_mu);
  r.error = save_cursor(conn, BX_CHAIN_PROJECT, 0, 0);
  pthread_mutex_unlock(&chain_mu);
  /* Pages already validated before this process are not seen in the cache.
   * Prune would delete them. The next full walk prunes. */
  r.allow_prune = from_start && r.error == NoError;
  return r;
}

BXChainResult bx_chain_sync_project(bXill *app, MYSQL *conn, Cache *cache) {
  BXChainResult r = {.error = NoError, .allow_prune = false};
  int offset = 0;
  bool from_start = false;
  PageCtx page = {.conn = conn, .step = BX_CHAIN_PROJECT, .pass = 0};
  BXChainAction action = begin_project(&offset);

  if (action == BX_CHAIN_SKIP) {
    bx_log_info("Sync chain skips projects; resume is at invoices");
    return r;
  }
  if (walk_project == NULL) {
    r.error = ErrorGeneric;
    return r;
  }
  if (action == BX_CHAIN_PLAIN) {
    r.error = walk_project(app, conn, cache, 0, NULL, NULL);
    r.allow_prune = (r.error == NoError);
    return r;
  }
  from_start = (offset == 0);
  r.error =
      walk_project(app, conn, cache, offset, on_validated_page, &page);
  if (r.error != NoError) {
    return r;
  }
  pthread_mutex_lock(&chain_mu);
  r.error = save_cursor(conn, BX_CHAIN_INVOICE, 0, 0);
  pthread_mutex_unlock(&chain_mu);
  r.allow_prune = from_start && r.error == NoError;
  return r;
}

static BXChainResult finish_invoice(MYSQL *conn, bool allow_prune) {
  BXChainResult r = {.error = NoError, .allow_prune = false};
  pthread_mutex_lock(&chain_mu);
  r.error = save_cursor(conn, BX_CHAIN_CONTACT, 0, 0);
  pthread_mutex_unlock(&chain_mu);
  r.allow_prune = allow_prune && r.error == NoError;
  return r;
}

BXChainResult bx_chain_sync_invoice(bXill *app, MYSQL *conn, Cache *cache) {
  BXChainResult r = {.error = NoError, .allow_prune = false};
  int offset = 0;
  int pass = 0;
  bool from_start = false;
  PageCtx page = {.conn = conn, .step = BX_CHAIN_INVOICE, .pass = 0};
  BXChainAction action = begin_invoice(&offset, &pass);

  if (walk_invoice == NULL || walk_missed == NULL) {
    r.error = ErrorGeneric;
    return r;
  }
  if (action == BX_CHAIN_PLAIN) {
    r.error = walk_invoice(app, conn, cache, 0, NULL, NULL);
    if (r.error != NoError) {
      return r;
    }
    r.error = walk_missed(app, conn, cache);
    r.allow_prune = (r.error == NoError);
    return r;
  }
  /* List already finished in a walk that started at 0. Do not list again.
   * Missed GETs are the ones that walk still owed, not pages we skipped. */
  if (pass == 1 || invoice_list_done) {
    if (!invoice_missed_done) {
      r.error = walk_missed(app, conn, cache);
      if (r.error != NoError) {
        return r;
      }
      invoice_missed_done = true;
    }
    return finish_invoice(conn, invoice_listed_here);
  }
  from_start = (offset == 0);
  r.error = walk_invoice(app, conn, cache, offset, on_validated_page, &page);
  if (r.error != NoError) {
    return r;
  }
  if (!from_start) {
    /* Skipped invoice pages were not seen. fetch-by-id would GET them. */
    return finish_invoice(conn, false);
  }
  pthread_mutex_lock(&chain_mu);
  invoice_listed_here = true;
  invoice_list_done = true;
  r.error = save_cursor(conn, BX_CHAIN_INVOICE, 1, 0);
  pthread_mutex_unlock(&chain_mu);
  if (r.error != NoError) {
    return r;
  }
  r.error = walk_missed(app, conn, cache);
  if (r.error != NoError) {
    return r;
  }
  invoice_missed_done = true;
  return finish_invoice(conn, true);
}
