#include "include/bx_chain.h"
#include "include/bx_utils.h"
#include "include/bx_walk.h"
#include "include/bxobjects/contact.h"
#include "include/bxobjects/invoice.h"
#include "include/bxobjects/project.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static BXChainCursor cursor = {.phase = BX_CHAIN_CONTACT_ACTIVE, .offset = 0};
/* Active contacts in this process started at offset 0 and finished. */
static bool contact_from_zero = false;
/* Invoice list in this process started at offset 0 and finished. Cleared when
 * that list starts and when the chain returns to contacts. */
static bool invoice_from_zero = false;

typedef struct {
  bXill *app;
  MYSQL *conn;
  BXChainPhase phase;
} PageCtx;

const char *bx_chain_phase_name(BXChainPhase phase) {
  switch (phase) {
  case BX_CHAIN_CONTACT_ARCHIVED:
    return "contact-archived";
  case BX_CHAIN_PROJECT:
    return "project";
  case BX_CHAIN_INVOICE_LIST:
    return "invoice-list";
  case BX_CHAIN_INVOICE_MISSED:
    return "invoice-missed";
  case BX_CHAIN_CONTACT_ACTIVE:
  default:
    return "contact-active";
  }
}

static bool parse_phase(const char *name, size_t len, BXChainPhase *phase) {
  static const BXChainPhase phases[] = {
      BX_CHAIN_CONTACT_ACTIVE, BX_CHAIN_CONTACT_ARCHIVED, BX_CHAIN_PROJECT,
      BX_CHAIN_INVOICE_LIST, BX_CHAIN_INVOICE_MISSED};
  size_t i = 0;

  for (i = 0; i < sizeof(phases) / sizeof(phases[0]); i++) {
    const char *known = bx_chain_phase_name(phases[i]);
    if (strlen(known) == len && strncmp(name, known, len) == 0) {
      *phase = phases[i];
      return true;
    }
  }
  return false;
}

static void normalize(BXChainCursor *c) {
  if (c->phase != BX_CHAIN_CONTACT_ACTIVE &&
      c->phase != BX_CHAIN_CONTACT_ARCHIVED && c->phase != BX_CHAIN_PROJECT &&
      c->phase != BX_CHAIN_INVOICE_LIST &&
      c->phase != BX_CHAIN_INVOICE_MISSED) {
    c->phase = BX_CHAIN_CONTACT_ACTIVE;
  }
  if (c->offset < 0) {
    c->offset = 0;
  }
  if (c->phase == BX_CHAIN_INVOICE_MISSED) {
    c->offset = 0;
  }
}

static bool parse_stored(const char *text, BXChainCursor *out) {
  const char *colon = NULL;
  char *end = NULL;
  long offset = 0;
  BXChainPhase phase = BX_CHAIN_CONTACT_ACTIVE;

  if (text == NULL) {
    return false;
  }
  colon = strchr(text, ':');
  if (colon == NULL || colon == text) {
    return false;
  }
  if (!parse_phase(text, (size_t)(colon - text), &phase)) {
    return false;
  }
  offset = strtol(colon + 1, &end, 10);
  if (end == colon + 1 || *end != '\0' || offset < 0 || offset > INT_MAX) {
    return false;
  }
  out->phase = phase;
  out->offset = (int)offset;
  normalize(out);
  return true;
}

static bool parse_legacy(const char *step, int offset, int pass,
                         BXChainCursor *out) {
  if (step == NULL) {
    return false;
  }
  if (offset < 0) {
    offset = 0;
  }
  if (strcmp(step, "contact") == 0) {
    out->phase =
        pass == 1 ? BX_CHAIN_CONTACT_ARCHIVED : BX_CHAIN_CONTACT_ACTIVE;
    out->offset = offset;
    return true;
  }
  if (strcmp(step, "project") == 0) {
    out->phase = BX_CHAIN_PROJECT;
    out->offset = offset;
    return true;
  }
  if (strcmp(step, "invoice") == 0) {
    if (pass == 1) {
      out->phase = BX_CHAIN_INVOICE_MISSED;
      out->offset = 0;
    } else {
      out->phase = BX_CHAIN_INVOICE_LIST;
      out->offset = offset;
    }
    return true;
  }
  return false;
}

void bx_chain_decode(const char *sync_chain, const char *legacy_step,
                     int legacy_offset, int legacy_pass, BXChainCursor *out) {
  if (out == NULL) {
    return;
  }
  out->phase = BX_CHAIN_CONTACT_ACTIVE;
  out->offset = 0;
  if (parse_stored(sync_chain, out)) {
    return;
  }
  if (!parse_legacy(legacy_step, legacy_offset, legacy_pass, out)) {
    out->phase = BX_CHAIN_CONTACT_ACTIVE;
    out->offset = 0;
  }
  normalize(out);
}

void bx_chain_init(bXill *app) {
  BXChainCursor loaded = {.phase = BX_CHAIN_CONTACT_ACTIVE, .offset = 0};

  if (!bx_chain_load(app, &loaded)) {
    loaded = (BXChainCursor){.phase = BX_CHAIN_CONTACT_ACTIVE, .offset = 0};
  }
  normalize(&loaded);
  cursor = loaded;
  contact_from_zero = false;
  invoice_from_zero = false;
  bx_log_info("Sync chain %s next offset %d", bx_chain_phase_name(loaded.phase),
              loaded.offset);
}

void bx_chain_read(BXChainCursor *out) {
  if (out == NULL) {
    return;
  }
  *out = cursor;
}

static BXillError checkpoint(bXill *app, MYSQL *conn, BXChainPhase phase,
                             int offset) {
  BXChainCursor next = {.phase = phase, .offset = offset};

  normalize(&next);
  if (!bx_chain_save(app, conn, &next)) {
    return ErrorGeneric;
  }
  cursor = next;
  return NoError;
}

/* The walk finished. Keep the next phase in memory when the row fails so a
 * retry does not list the same pages again. */
static BXillError advance_phase(bXill *app, MYSQL *conn, BXChainPhase phase) {
  BXChainCursor next = {.phase = phase, .offset = 0};

  normalize(&next);
  cursor = next;
  if (!bx_chain_save(app, conn, &next)) {
    return ErrorGeneric;
  }
  return NoError;
}

static BXillError on_validated_page(void *v, int64_t next_offset) {
  PageCtx *page = v;

  if (page == NULL || next_offset < 0 || next_offset > INT_MAX) {
    return ErrorGeneric;
  }
  return checkpoint(page->app, page->conn, page->phase, (int)next_offset);
}

static BXChainResult fail_resource(BXillError error, bool contact, bool project,
                                   bool invoice) {
  BXChainResult r = {.error = error};
  r.bump_contact = contact;
  r.bump_project = project;
  r.bump_invoice = invoice;
  return r;
}

static BXChainResult finish_resource(bXill *app, MYSQL *conn, BXChainPhase next,
                                     bool contact, bool project, bool invoice) {
  BXChainResult r = {0};

  r.error = advance_phase(app, conn, next);
  r.bump_contact = contact;
  r.bump_project = project;
  r.bump_invoice = invoice;
  r.pause = (r.error == NoError);
  return r;
}

BXChainResult bx_chain_sync(bXill *app, MYSQL *conn, Cache *contacts,
                            Cache *projects, Cache *invoices) {
  BXChainPhase phase = cursor.phase;
  int offset = cursor.offset;
  bool from_zero = false;
  PageCtx page = {.app = app, .conn = conn, .phase = phase};
  BXChainResult r = {0};

  if (app == NULL || conn == NULL) {
    return fail_resource(ErrorGeneric, false, false, false);
  }

  switch (phase) {
  case BX_CHAIN_CONTACT_ACTIVE:
    contact_from_zero = false;
    from_zero = (offset == 0);
    r.error = bx_contact_walk_pass(app, conn, contacts, false, offset,
                                   on_validated_page, &page);
    if (r.error != NoError) {
      return fail_resource(r.error, true, false, false);
    }
    if (from_zero) {
      contact_from_zero = true;
    }
    r = finish_resource(app, conn, BX_CHAIN_CONTACT_ARCHIVED, false, false,
                        false);
    /* Archived is the rest of this resource. Bump only if the row failed. */
    r.bump_contact = (r.error != NoError);
    r.pause = false;
    return r;

  case BX_CHAIN_CONTACT_ARCHIVED:
    r.error = bx_contact_walk_pass(app, conn, contacts, true, offset,
                                   on_validated_page, &page);
    if (r.error != NoError) {
      return fail_resource(r.error, true, false, false);
    }
    r = finish_resource(app, conn, BX_CHAIN_PROJECT, true, false, false);
    if (r.error == NoError) {
      r.prune_contact = contact_from_zero;
    }
    contact_from_zero = false;
    return r;

  case BX_CHAIN_PROJECT:
    from_zero = (offset == 0);
    r.error = bx_project_walk_item_from(app, conn, projects, offset,
                                        on_validated_page, &page);
    if (r.error != NoError) {
      return fail_resource(r.error, false, true, false);
    }
    r = finish_resource(app, conn, BX_CHAIN_INVOICE_LIST, false, true, false);
    if (r.error == NoError) {
      r.prune_project = from_zero;
    }
    return r;

  case BX_CHAIN_INVOICE_LIST:
    invoice_from_zero = false;
    from_zero = (offset == 0);
    r.error = bx_invoice_walk_list(app, conn, invoices, offset,
                                   on_validated_page, &page);
    if (r.error != NoError) {
      return fail_resource(r.error, false, false, true);
    }
    if (!from_zero) {
      /* Pages before this offset were not seen. Missed-ids would GET them. */
      return finish_resource(app, conn, BX_CHAIN_CONTACT_ACTIVE, false, false,
                             true);
    }
    invoice_from_zero = true;
    r = finish_resource(app, conn, BX_CHAIN_INVOICE_MISSED, false, false, false);
    r.bump_invoice = (r.error != NoError);
    r.pause = false;
    return r;

  case BX_CHAIN_INVOICE_MISSED:
    r.error = bx_invoice_fetch_missed(app, conn, invoices);
    if (r.error != NoError) {
      return fail_resource(r.error, false, false, true);
    }
    from_zero = invoice_from_zero;
    invoice_from_zero = false;
    r = finish_resource(app, conn, BX_CHAIN_CONTACT_ACTIVE, false, false, true);
    if (r.error == NoError) {
      r.prune_invoice = from_zero;
    }
    return r;
  }

  return fail_resource(ErrorGeneric, false, false, false);
}
