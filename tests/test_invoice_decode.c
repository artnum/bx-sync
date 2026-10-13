#include "../src/include/bx_database.h"
#include "../src/include/bx_utils.h"
#include "../src/include/bx_walk.h"
#include "../src/include/bxobjects/contact.h"
#include "../src/include/bxobjects/invoice.h"
#include "../src/include/bxobjects/project.h"

#include <stdio.h>
#include <string.h>

/* Linked only so invoice.c resolves. The test never calls them. */
void _bx_log_debug(char *file, int line, pthread_t thid, const char *fmt, ...) {
  (void)file;
  (void)line;
  (void)thid;
  (void)fmt;
}

BXNetRequest *bx_do_request(BXNetRequestList *queue, json_t *body,
                            char *path_fmt, ...) {
  (void)queue;
  (void)body;
  (void)path_fmt;
  return NULL;
}

void bx_net_request_free(BXNetRequest *request) { (void)request; }

BXillError bx_contact_is_in_database(MYSQL *conn, BXGeneric *item,
                                     bool *present) {
  (void)conn;
  (void)item;
  (void)present;
  return NoError;
}

BXillError bx_project_is_in_database(MYSQL *conn, BXGeneric *item,
                                     bool *present) {
  (void)conn;
  (void)item;
  (void)present;
  return NoError;
}

BXDatabaseQuery *bx_database_new_query(MYSQL *mysql, const char *query) {
  (void)mysql;
  (void)query;
  return NULL;
}

bool bx_database_execute(BXDatabaseQuery *query) {
  (void)query;
  return false;
}

bool bx_database_results(BXDatabaseQuery *query) {
  (void)query;
  return false;
}

void bx_database_free_query(BXDatabaseQuery *query) { (void)query; }

bool bx_database_add_bxtype(BXDatabaseQuery *query, const char *name,
                            const BXGeneric *value) {
  (void)query;
  (void)name;
  (void)value;
  return false;
}

bool bx_database_add_param_int(BXDatabaseQuery *query, const char *name,
                               void *value, size_t value_len,
                               enum enum_field_types type, bool is_unsigned) {
  (void)query;
  (void)name;
  (void)value;
  (void)value_len;
  (void)type;
  (void)is_unsigned;
  return false;
}

BXillError bx_invoice_positions_store(MYSQL *conn, uint64_t invoice_id,
                                      json_t *positions) {
  (void)conn;
  (void)invoice_id;
  (void)positions;
  return NoError;
}

BXillError bx_invoice_extra_sync(bXill *app, MYSQL *conn, uint64_t invoice_id) {
  (void)app;
  (void)conn;
  (void)invoice_id;
  return NoError;
}

CacheState cache_check_item(Cache *c, uint64_t id, uint64_t checksum) {
  (void)c;
  (void)id;
  (void)checksum;
  return CacheNotSet;
}

bool cache_set_item(Cache *c, uint64_t id, uint64_t checksum) {
  (void)c;
  (void)id;
  (void)checksum;
  return false;
}

void cache_iter_init(Cache *c, CacheIter *iter) {
  (void)c;
  (void)iter;
}

bool cache_iter_next_prunable_id(CacheIter *iter, uint64_t drift, uint64_t *id) {
  (void)iter;
  (void)drift;
  (void)id;
  return false;
}

BXillError bx_walk_pages(bXill *app, MYSQL *conn, const char *path_fmt,
                         BXGeneric *extra, const struct timespec *page_sleep,
                         bx_walk_sync_fn sync, void *ctx) {
  (void)app;
  (void)conn;
  (void)path_fmt;
  (void)extra;
  (void)page_sleep;
  (void)sync;
  (void)ctx;
  return NoError;
}

BXillError bx_walk_pages_from(bXill *app, MYSQL *conn, const char *path_fmt,
                              BXGeneric *extra, const struct timespec *page_sleep,
                              bx_walk_sync_fn sync, void *ctx,
                              int64_t start_offset, bx_walk_page_fn on_page,
                              void *page_ctx) {
  (void)app;
  (void)conn;
  (void)path_fmt;
  (void)extra;
  (void)page_sleep;
  (void)sync;
  (void)ctx;
  (void)start_offset;
  (void)on_page;
  (void)page_ctx;
  return NoError;
}

/* Same bytes the JSON readers mix into XXH3, and only when the field is set.
 * Order is the decode order. An extra pass over mwst_type, the totals, or
 * tva_is_net makes this differ from invoice->checksum. */
static void mix_uint(XXH3_state_t *state, const BXUInteger *value) {
  if (!value->isset) {
    return;
  }
  XXH3_64bits_update(state, &value->value, sizeof(value->value));
}

static void mix_float(XXH3_state_t *state, const BXFloat *value) {
  if (!value->isset) {
    return;
  }
  XXH3_64bits_update(state, &value->value, sizeof(value->value));
}

static void mix_string(XXH3_state_t *state, const BXString *value) {
  if (!value->isset || value->value == NULL) {
    return;
  }
  XXH3_64bits_update(state, value->value, value->value_len);
}

static uint64_t hash_fields_once(const BXObjectInvoice *invoice) {
  XXH3_state_t *state = XXH3_createState();
  uint64_t checksum = 0;
  if (state == NULL) {
    return 0;
  }
  XXH3_64bits_reset(state);
  mix_uint(state, &invoice->id);
  mix_uint(state, &invoice->user_id);
  mix_uint(state, &invoice->contact_id);
  mix_uint(state, &invoice->contact_sub_id);
  mix_uint(state, &invoice->project_id);
  mix_uint(state, &invoice->bank_account_id);
  mix_uint(state, &invoice->currency_id);
  mix_uint(state, &invoice->payment_type_id);
  mix_uint(state, &invoice->kb_item_status_id);
  mix_uint(state, &invoice->esr_id);
  mix_uint(state, &invoice->qr_invoice_id);
  mix_uint(state, &invoice->language_id);
  mix_uint(state, &invoice->mwst_type);
  mix_uint(state, &invoice->show_position_taxes);
  mix_uint(state, &invoice->mwst_is_net);
  mix_float(state, &invoice->total_gross);
  mix_float(state, &invoice->total_net);
  mix_float(state, &invoice->total_taxes);
  mix_float(state, &invoice->total_received_payments);
  mix_float(state, &invoice->total_credit_vouchers);
  mix_float(state, &invoice->total_remaining_payments);
  mix_float(state, &invoice->total);
  mix_float(state, &invoice->total_rounding_difference);
  mix_string(state, &invoice->document_nr);
  mix_string(state, &invoice->is_valid_from);
  mix_string(state, &invoice->is_valid_to);
  mix_string(state, &invoice->contact_address);
  mix_string(state, &invoice->template_slug);
  mix_string(state, &invoice->updated_at);
  mix_string(state, &invoice->reference);
  mix_string(state, &invoice->api_reference);
  mix_string(state, &invoice->viewed_by_client_at);
  mix_string(state, &invoice->header);
  mix_string(state, &invoice->footer);
  mix_string(state, &invoice->title);
  mix_string(state, &invoice->network_link);
  checksum = XXH3_64bits_digest(state);
  XXH3_freeState(state);
  return checksum;
}

static const char FIXTURE[] =
    "{"
    "\"id\":42,"
    "\"user_id\":7,"
    "\"contact_id\":1001,"
    "\"contact_sub_id\":null,"
    "\"project_id\":0,"
    "\"bank_account_id\":3,"
    "\"currency_id\":1,"
    "\"payment_type_id\":4,"
    "\"kb_item_status_id\":8,"
    "\"esr_id\":null,"
    "\"qr_invoice_id\":9,"
    "\"language_id\":2,"
    "\"mwst_type\":1,"
    "\"show_position_taxes\":0,"
    "\"mwst_is_net\":true,"
    "\"total_gross\":120.5,"
    "\"total_net\":111.57,"
    "\"total_taxes\":8.93,"
    "\"total_received_payments\":0.0,"
    "\"total_credit_vouchers\":0.0,"
    "\"total_remaining_payments\":120.5,"
    "\"total\":120.5,"
    "\"total_rounding_difference\":0.0,"
    "\"document_nr\":\"RE-TEST-1\","
    "\"is_valid_from\":\"2026-01-15\","
    "\"is_valid_to\":\"2026-02-14\","
    "\"contact_address\":\"Example SA\","
    "\"template_slug\":\"tpl\","
    "\"updated_at\":\"2026-01-15 10:00:00\","
    "\"reference\":\"REF-1\","
    "\"api_reference\":\"api-1\","
    "\"viewed_by_client_at\":null,"
    "\"header\":\"Hello\","
    "\"footer\":\"Thanks\","
    "\"title\":\"Fictional invoice\","
    "\"network_link\":\"https://example.invalid/i/1\","
    "\"tva_is_net\":true"
    "}";

static int g_failed = 0;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_failed = 1;
  }
}

static BXObjectInvoice *decode(json_t *root) {
  return (BXObjectInvoice *)bx_object_invoice_decode(root);
}

int main(void) {
  json_error_t error;
  json_t *root = NULL;
  json_t *without_tva = NULL;
  json_t *other_gross = NULL;
  BXObjectInvoice *invoice = NULL;
  BXObjectInvoice *untouched = NULL;
  BXObjectInvoice *changed = NULL;
  uint64_t once = 0;

  root = json_loads(FIXTURE, 0, &error);
  if (root == NULL) {
    fprintf(stderr, "FAIL: fixture JSON: %s\n", error.text);
    return 1;
  }
  invoice = decode(root);
  expect(invoice != NULL, "decode returned NULL");
  if (invoice == NULL) {
    json_decref(root);
    return 1;
  }

  expect(invoice->mwst_is_net.isset, "mwst_is_net is not set");
  expect(invoice->mwst_is_net.value == 1, "mwst_is_net true did not become 1");
  expect(invoice->mwst_is_net.type == BX_OBJECT_TYPE_UINTEGER,
         "mwst_is_net is not an unsigned integer");
  expect(invoice->mwst_type.isset && invoice->mwst_type.value == 1,
         "mwst_type was not read");
  expect(invoice->total_gross.isset && invoice->total_gross.value == 120.5,
         "total_gross was not stored");

  once = hash_fields_once(invoice);
  if (invoice->checksum != once) {
    fprintf(stderr,
            "FAIL: checksum %llx != single-pass %llx "
            "(duplicate field or tva_is_net still hashed)\n",
            (unsigned long long)invoice->checksum, (unsigned long long)once);
    g_failed = 1;
  }

  without_tva = json_deep_copy(root);
  expect(without_tva != NULL && json_object_del(without_tva, "tva_is_net") == 0,
         "could not drop tva_is_net");
  if (without_tva != NULL) {
    untouched = decode(without_tva);
    expect(untouched != NULL, "decode without tva_is_net returned NULL");
    if (untouched != NULL) {
      expect(untouched->checksum == invoice->checksum,
             "tva_is_net key changed the checksum");
      expect(untouched->mwst_is_net.isset && untouched->mwst_is_net.value == 1,
             "mwst_is_net lost when tva_is_net is absent");
    }
  }

  other_gross = json_deep_copy(root);
  expect(other_gross != NULL, "could not copy fixture");
  if (other_gross != NULL) {
    json_object_set_new(other_gross, "total_gross", json_real(1.25));
    changed = decode(other_gross);
    expect(changed != NULL, "decode with other total_gross returned NULL");
    if (changed != NULL) {
      expect(changed->total_gross.isset && changed->total_gross.value == 1.25,
             "replaced total_gross was not stored");
      expect(changed->checksum != invoice->checksum,
             "changing only total_gross left the checksum unchanged");
      expect(changed->mwst_is_net.isset && changed->mwst_is_net.value == 1,
             "mwst_is_net lost when total_gross changed");
    }
  }

  bx_object_invoice_free(changed);
  bx_object_invoice_free(untouched);
  bx_object_invoice_free(invoice);
  json_decref(other_gross);
  json_decref(without_tva);
  json_decref(root);

  if (g_failed) {
    return 1;
  }
  printf("ok\n");
  return 0;
}
