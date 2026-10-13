#include "../src/include/bx_chain.h"
#include "../src/include/bx_object_value.h"
#include "../src/include/bx_utils.h"
#include "../src/include/bxill.h"

#include <jansson.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* No MySQL and no network. bx_do_request is a counter, progress is in memory.
 * A full cycle must match bx_walk_pages: one request per data page, plus the
 * empty page that ends the walk, and nothing else. */

void _bx_log_debug(char *file, int line, pthread_t thid, const char *fmt, ...) {
  (void)file;
  (void)line;
  (void)thid;
  (void)fmt;
}

void _bx_log_info(char *file, int line, pthread_t thid, const char *fmt, ...) {
  (void)file;
  (void)line;
  (void)thid;
  (void)fmt;
}

void _bx_log_error(char *file, int line, pthread_t thid, const char *fmt, ...) {
  (void)file;
  (void)line;
  (void)thid;
  (void)fmt;
}

enum { KIND_CONTACT = 0, KIND_PROJECT = 1, KIND_INVOICE = 2 };

typedef struct {
  int kind;
  int archived;
  int64_t offset;
  int64_t limit;
} HttpCall;

static HttpCall calls[128];
static int ncalls;
static int call_overflow;
static int data_pages[3][2];
static int missed_calls;
static int missed_fail;
static int save_fail;
/* Succeed this many stores, then fail. -1 disables the cap. */
static int saves_before_fail = -1;
static int saves_done = 0;
static int poison_kind = -1;
static int poison_page = -1;
static int g_poison;

static BXChainCursor saved;
static bool saved_present;

static int failures;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
      failures++;                                                              \
    }                                                                          \
  } while (0)

static bool mem_load(void *ctx, BXChainCursor *out) {
  (void)ctx;
  if (!saved_present) {
    out->step = BX_CHAIN_CONTACT;
    out->offset = 0;
    out->pass = 0;
    return true;
  }
  *out = saved;
  return true;
}

static bool mem_save(void *ctx, MYSQL *conn, const BXChainCursor *in) {
  (void)ctx;
  (void)conn;
  if (save_fail) {
    return false;
  }
  if (saves_before_fail >= 0 && saves_done >= saves_before_fail) {
    return false;
  }
  saves_done++;
  saved = *in;
  saved_present = true;
  return true;
}

static int kind_of(const char *path) {
  if (strstr(path, "contact") != NULL) {
    return KIND_CONTACT;
  }
  if (strstr(path, "pr_project") != NULL) {
    return KIND_PROJECT;
  }
  if (strstr(path, "kb_invoice") != NULL) {
    return KIND_INVOICE;
  }
  return -1;
}

BXNetRequest *bx_do_request(BXNetRequestList *queue, json_t *body,
                            char *path_fmt, ...) {
  va_list ap;
  BXInteger *limit = NULL;
  BXInteger *offset = NULL;
  int kind = 0;
  int archived = 0;
  int slot = 0;
  int page_index = 0;
  int pages = 0;
  json_t *arr = NULL;
  json_t *item = NULL;
  BXNetRequest *request = NULL;

  (void)queue;
  (void)body;
  va_start(ap, path_fmt);
  limit = (BXInteger *)va_arg(ap, BXGeneric *);
  offset = (BXInteger *)va_arg(ap, BXGeneric *);
  if (strstr(path_fmt, "show_archived=$") != NULL) {
    BXBool *flag = (BXBool *)va_arg(ap, BXGeneric *);
    archived = flag != NULL && flag->value ? 1 : 0;
  }
  va_end(ap);

  kind = kind_of(path_fmt);
  if (limit == NULL || offset == NULL || kind < 0 ||
      limit->value != BXILL_LIST_LIMIT) {
    call_overflow = 1;
    return NULL;
  }
  if (ncalls >= (int)(sizeof(calls) / sizeof(calls[0]))) {
    call_overflow = 1;
    return NULL;
  }
  calls[ncalls].kind = kind;
  calls[ncalls].archived = archived;
  calls[ncalls].offset = offset->value;
  calls[ncalls].limit = limit->value;
  ncalls++;

  slot = (kind == KIND_CONTACT && archived) ? 1 : 0;
  pages = data_pages[kind][slot];
  page_index = (int)(offset->value / BXILL_LIST_LIMIT);
  g_poison = (kind == poison_kind && page_index == poison_page &&
              page_index < pages);

  arr = json_array();
  if (arr == NULL) {
    return NULL;
  }
  if (page_index < pages) {
    item = json_pack("{s:i}", "id", page_index);
    if (item == NULL || json_array_append_new(arr, item) != 0) {
      json_decref(arr);
      return NULL;
    }
  }
  request = calloc(1, sizeof(*request));
  if (request == NULL) {
    json_decref(arr);
    return NULL;
  }
  request->decoded = arr;
  return request;
}

void bx_net_request_free(BXNetRequest *request) {
  if (request == NULL) {
    return;
  }
  json_decref(request->decoded);
  free(request);
}

static BXillError test_sync(bXill *app, MYSQL *conn, json_t *item, void *ctx) {
  (void)app;
  (void)conn;
  (void)item;
  (void)ctx;
  if (g_poison) {
    return ErrorNet;
  }
  return NoError;
}

static BXillError test_contact(bXill *app, MYSQL *conn, Cache *cache,
                               bool archived, int64_t start_offset,
                               bx_walk_page_fn on_page, void *page_ctx) {
  BXBool flag = {.type = BX_OBJECT_TYPE_BOOL, .isset = true, .value = archived};
  (void)cache;
  return bx_walk_pages_from(app, conn,
                            "2.0/contact?limit=$&offset=$&show_archived=$",
                            (BXGeneric *)&flag, NULL, test_sync, NULL,
                            start_offset, on_page, page_ctx);
}

static BXillError test_project(bXill *app, MYSQL *conn, Cache *cache,
                               int64_t start_offset, bx_walk_page_fn on_page,
                               void *page_ctx) {
  (void)cache;
  return bx_walk_pages_from(app, conn, "2.0/pr_project?limit=$&offset=$", NULL,
                            NULL, test_sync, NULL, start_offset, on_page,
                            page_ctx);
}

static BXillError test_invoice(bXill *app, MYSQL *conn, Cache *cache,
                               int64_t start_offset, bx_walk_page_fn on_page,
                               void *page_ctx) {
  (void)cache;
  return bx_walk_pages_from(
      app, conn, "2.0/kb_invoice?limit=$&offset=$&order_by=id", NULL, NULL,
      test_sync, NULL, start_offset, on_page, page_ctx);
}

static BXillError test_missed(bXill *app, MYSQL *conn, Cache *cache) {
  (void)app;
  (void)conn;
  (void)cache;
  missed_calls++;
  if (missed_fail) {
    return ErrorNet;
  }
  return NoError;
}

static void reset_http(void) {
  ncalls = 0;
  call_overflow = 0;
  g_poison = 0;
  memset(calls, 0, sizeof(calls));
}

static void set_pages(int live, int archived, int project, int invoice) {
  data_pages[KIND_CONTACT][0] = live;
  data_pages[KIND_CONTACT][1] = archived;
  data_pages[KIND_PROJECT][0] = project;
  data_pages[KIND_INVOICE][0] = invoice;
}

static int expected_lists(void) {
  return (data_pages[KIND_CONTACT][0] + 1) + (data_pages[KIND_CONTACT][1] + 1) +
         (data_pages[KIND_PROJECT][0] + 1) + (data_pages[KIND_INVOICE][0] + 1);
}

static int count_kind(int kind) {
  int n = 0;
  int i = 0;
  for (i = 0; i < ncalls; i++) {
    if (calls[i].kind == kind) {
      n++;
    }
  }
  return n;
}

static void boot(bXill *app) {
  (void)app;
  poison_kind = -1;
  poison_page = -1;
  missed_fail = 0;
  save_fail = 0;
  saves_before_fail = -1;
  missed_calls = 0;
  reset_http();
  bx_chain_set_store(mem_load, mem_save, NULL);
  bx_chain_set_walks(test_contact, test_project, test_invoice, test_missed);
  bx_chain_init();
}

static void old_lists(bXill *app, MYSQL *conn) {
  BXBool archived = {
      .type = BX_OBJECT_TYPE_BOOL, .isset = true, .value = false};
  bx_walk_pages(app, conn, "2.0/contact?limit=$&offset=$&show_archived=$",
                (BXGeneric *)&archived, NULL, test_sync, NULL);
  archived.value = true;
  bx_walk_pages(app, conn, "2.0/contact?limit=$&offset=$&show_archived=$",
                (BXGeneric *)&archived, NULL, test_sync, NULL);
  bx_walk_pages(app, conn, "2.0/pr_project?limit=$&offset=$", NULL, NULL,
                test_sync, NULL);
  bx_walk_pages(app, conn, "2.0/kb_invoice?limit=$&offset=$&order_by=id", NULL,
                NULL, test_sync, NULL);
}

static void test_full_cycle_matches_old_walker(bXill *app, MYSQL *conn) {
  HttpCall old[128];
  int old_n = 0;
  BXChainCursor cur;
  BXChainResult contact;
  BXChainResult project;
  BXChainResult invoice;

  set_pages(2, 1, 3, 2);
  saved_present = false;
  reset_http();
  old_lists(app, conn);
  old_n = ncalls;
  memcpy(old, calls, sizeof(calls));
  CHECK(call_overflow == 0);
  CHECK(old_n == expected_lists());
  CHECK(old[0].kind == KIND_CONTACT);
  CHECK(old[0].offset == 0);
  CHECK(old[0].archived == 0);

  saved_present = false;
  boot(app);
  CHECK(saved_present == false);
  contact = bx_chain_sync_contact(app, conn, NULL);
  project = bx_chain_sync_project(app, conn, NULL);
  invoice = bx_chain_sync_invoice(app, conn, NULL);
  CHECK(contact.error == NoError);
  CHECK(project.error == NoError);
  CHECK(invoice.error == NoError);
  CHECK(contact.allow_prune);
  CHECK(project.allow_prune);
  CHECK(invoice.allow_prune);
  CHECK(call_overflow == 0);
  CHECK(ncalls == old_n);
  CHECK(memcmp(calls, old, (size_t)old_n * sizeof(HttpCall)) == 0);
  CHECK(missed_calls == 1);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_CONTACT);
  CHECK(cur.offset == 0);
  CHECK(cur.pass == 0);
  CHECK(saved_present);
  CHECK(saved.step == BX_CHAIN_CONTACT);
  CHECK(saved.offset == 0);
  CHECK(saved.pass == 0);
}

static void test_resume_mid_project(bXill *app, MYSQL *conn) {
  BXChainCursor cur;
  BXChainResult contact;
  BXChainResult project;
  BXChainResult invoice;
  int i = 0;

  set_pages(1, 1, 4, 1);
  saved_present = false;
  boot(app);
  contact = bx_chain_sync_contact(app, conn, NULL);
  CHECK(contact.error == NoError);
  poison_kind = KIND_PROJECT;
  poison_page = 2;
  project = bx_chain_sync_project(app, conn, NULL);
  CHECK(project.error != NoError);
  CHECK(!project.allow_prune);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_PROJECT);
  CHECK(cur.offset == 2 * BXILL_LIST_LIMIT);
  CHECK(cur.pass == 0);

  boot(app);
  contact = bx_chain_sync_contact(app, conn, NULL);
  CHECK(contact.error == NoError);
  CHECK(!contact.allow_prune);
  CHECK(count_kind(KIND_CONTACT) == 0);
  project = bx_chain_sync_project(app, conn, NULL);
  CHECK(project.error == NoError);
  CHECK(!project.allow_prune);
  CHECK(ncalls > 0);
  CHECK(calls[0].kind == KIND_PROJECT);
  CHECK(calls[0].offset == 2 * BXILL_LIST_LIMIT);
  for (i = 0; i < ncalls; i++) {
    CHECK(calls[i].kind != KIND_CONTACT);
    if (calls[i].kind == KIND_PROJECT) {
      CHECK(calls[i].offset >= 2 * BXILL_LIST_LIMIT);
    }
  }
  invoice = bx_chain_sync_invoice(app, conn, NULL);
  CHECK(invoice.error == NoError);
  CHECK(invoice.allow_prune);
  CHECK(missed_calls == 1);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_CONTACT);
  CHECK(cur.offset == 0);
  CHECK(cur.pass == 0);
}

static void test_page_error_does_not_advance(bXill *app, MYSQL *conn) {
  BXChainCursor cur;
  BXChainResult project;

  set_pages(1, 1, 3, 1);
  saved_present = false;
  boot(app);
  CHECK(bx_chain_sync_contact(app, conn, NULL).error == NoError);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_PROJECT);
  CHECK(cur.offset == 0);

  poison_kind = KIND_PROJECT;
  poison_page = 0;
  project = bx_chain_sync_project(app, conn, NULL);
  CHECK(project.error != NoError);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_PROJECT);
  CHECK(cur.offset == 0);
  CHECK(cur.pass == 0);

  saved_present = false;
  boot(app);
  CHECK(bx_chain_sync_contact(app, conn, NULL).error == NoError);
  poison_kind = KIND_PROJECT;
  poison_page = 1;
  project = bx_chain_sync_project(app, conn, NULL);
  CHECK(project.error != NoError);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_PROJECT);
  CHECK(cur.offset == BXILL_LIST_LIMIT);
  CHECK(cur.pass == 0);
}

static void test_resume_mid_invoice_skips_missed(bXill *app, MYSQL *conn) {
  BXChainCursor cur;
  BXChainResult contact;
  BXChainResult project;
  BXChainResult invoice;

  set_pages(1, 1, 1, 3);
  saved_present = false;
  boot(app);
  CHECK(bx_chain_sync_contact(app, conn, NULL).error == NoError);
  CHECK(bx_chain_sync_project(app, conn, NULL).error == NoError);
  poison_kind = KIND_INVOICE;
  poison_page = 1;
  invoice = bx_chain_sync_invoice(app, conn, NULL);
  CHECK(invoice.error != NoError);
  CHECK(missed_calls == 0);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_INVOICE);
  CHECK(cur.offset == BXILL_LIST_LIMIT);
  CHECK(cur.pass == 0);

  boot(app);
  contact = bx_chain_sync_contact(app, conn, NULL);
  project = bx_chain_sync_project(app, conn, NULL);
  CHECK(contact.error == NoError);
  CHECK(project.error == NoError);
  CHECK(ncalls == 0);
  invoice = bx_chain_sync_invoice(app, conn, NULL);
  CHECK(invoice.error == NoError);
  CHECK(!invoice.allow_prune);
  CHECK(missed_calls == 0);
  CHECK(calls[0].kind == KIND_INVOICE);
  CHECK(calls[0].offset == BXILL_LIST_LIMIT);
  CHECK(count_kind(KIND_CONTACT) == 0);
  CHECK(count_kind(KIND_PROJECT) == 0);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_CONTACT);
  CHECK(cur.offset == 0);
}

static void test_missed_error_does_not_relist(bXill *app, MYSQL *conn) {
  BXChainCursor cur;
  int listed = 0;

  set_pages(1, 0, 1, 2);
  saved_present = false;
  boot(app);
  CHECK(bx_chain_sync_contact(app, conn, NULL).error == NoError);
  CHECK(bx_chain_sync_project(app, conn, NULL).error == NoError);
  reset_http();
  missed_fail = 1;
  CHECK(bx_chain_sync_invoice(app, conn, NULL).error != NoError);
  listed = ncalls;
  CHECK(listed == data_pages[KIND_INVOICE][0] + 1);
  CHECK(missed_calls == 1);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_INVOICE);
  CHECK(cur.pass == 1);

  missed_fail = 0;
  boot(app);
  CHECK(bx_chain_sync_contact(app, conn, NULL).error == NoError);
  CHECK(bx_chain_sync_project(app, conn, NULL).error == NoError);
  CHECK(ncalls == 0);
  CHECK(bx_chain_sync_invoice(app, conn, NULL).error == NoError);
  CHECK(ncalls == 0);
  CHECK(missed_calls == 1);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_CONTACT);
  CHECK(cur.offset == 0);
  CHECK(cur.pass == 0);
}

static void test_marker_save_failure_does_not_relist(bXill *app, MYSQL *conn) {
  BXChainCursor cur;

  set_pages(1, 0, 1, 1);
  saved_present = false;
  boot(app);
  CHECK(bx_chain_sync_contact(app, conn, NULL).error == NoError);
  CHECK(bx_chain_sync_project(app, conn, NULL).error == NoError);
  reset_http();
  missed_calls = 0;
  saves_done = 0;
  /* Page checkpoint succeeds; the "list done" row fails. */
  saves_before_fail = 1;
  CHECK(bx_chain_sync_invoice(app, conn, NULL).error != NoError);
  CHECK(ncalls == data_pages[KIND_INVOICE][0] + 1);
  CHECK(missed_calls == 0);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_INVOICE);
  CHECK(cur.pass == 0);
  CHECK(cur.offset == BXILL_LIST_LIMIT);

  saves_before_fail = -1;
  reset_http();
  missed_calls = 0;
  CHECK(bx_chain_sync_invoice(app, conn, NULL).error == NoError);
  CHECK(ncalls == 0);
  CHECK(missed_calls == 1);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_CONTACT);
  CHECK(cur.offset == 0);
  CHECK(cur.pass == 0);
}

static void test_save_failure_does_not_advance(bXill *app, MYSQL *conn) {
  BXChainCursor cur;

  set_pages(2, 1, 1, 1);
  saved_present = false;
  boot(app);
  save_fail = 1;
  CHECK(bx_chain_sync_contact(app, conn, NULL).error != NoError);
  bx_chain_read(&cur);
  CHECK(cur.step == BX_CHAIN_CONTACT);
  CHECK(cur.offset == 0);
  CHECK(cur.pass == 0);
  CHECK(!saved_present);
}

int main(void) {
  bXill app;
  MYSQL *conn = (MYSQL *)1;

  memset(&app, 0, sizeof(app));
  test_full_cycle_matches_old_walker(&app, conn);
  test_resume_mid_project(&app, conn);
  test_page_error_does_not_advance(&app, conn);
  test_resume_mid_invoice_skips_missed(&app, conn);
  test_missed_error_does_not_relist(&app, conn);
  test_marker_save_failure_does_not_relist(&app, conn);
  test_save_failure_does_not_advance(&app, conn);
  if (failures != 0) {
    fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  printf("sync chain resume ok\n");
  return 0;
}
