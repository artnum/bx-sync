#include "../include/bxobjects/invoice.h"
#include "../include/bx_database.h"
#include "../include/bx_ids_cache.h"
#include "../include/bx_object.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"
#include "../include/bxobjects/contact.h"
#include "../include/bxobjects/project.h"
#include <assert.h>
#include <jansson.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#define QUERY_INSERT                                                           \
  "INSERT IGNORE INTO invoice (id, document_nr, title, contact_id, "           \
  "contact_sub_id, "                                                           \
  "user_id, project_id, language_id, bank_account_id, currency_id, "           \
  "payment_type_id, header, footer, mwst_type, mwst_is_net, "                  \
  "show_position_taxes, is_valid_from, is_valid_to, contact_address, "         \
  "kb_item_status_id, reference, api_reference, viewed_by_client_at, "         \
  "updated_at, esr_id, qr_invoice_id, template_slug, network_link, "           \
  "_checksum, _last_updated, total_gross, total_net, total_taxes, "            \
  "total_received_payments, total_credit_vouchers, total_remaining_payments, " \
  "total, total_rounding_difference) "                                         \
  "VALUES (:id, :document_nr, :title, "                                        \
  ":contact_id, :contact_sub_id, :user_id, :project_id, :language_id, "        \
  ":bank_account_id, :currency_id, :payment_type_id, :header, :footer, "       \
  ":mwst_type, :mwst_is_net, :show_position_taxes, :is_valid_from, "           \
  ":is_valid_to, :contact_address, :kb_item_status_id, :reference, "           \
  ":api_reference, :viewed_by_client_at, :updated_at, :esr_id, "               \
  ":qr_invoice_id, :template_slug, :network_link, :_checksum, "                \
  ":_last_updated, :total_gross, :total_net, :total_taxes, "                   \
  ":total_received_payments, :total_credit_vouchers, "                         \
  ":total_remaining_payments, :total, :total_rounding_difference "             \
  ");"

#define QUERY_UPDATE                                                           \
  "UPDATE invoice SET document_nr = :document_nr, title = :title, "            \
  "contact_id = :contact_id, contact_sub_id = :contact_sub_id, "               \
  "user_id = :user_id, project_id = :project_id, language_id = :language_id, " \
  "bank_account_id = :bank_account_id, currency_id = :currency_id, "           \
  "payment_type_id = :payment_type_id, header = :header, footer = :footer, "   \
  "mwst_type = :mwst_type, mwst_is_net = :mwst_is_net, "                       \
  "show_position_taxes = :show_position_taxes, is_valid_from = "               \
  ":is_valid_from,"                                                            \
  "is_valid_to = :is_valid_to, contact_address = :contact_address, "           \
  "kb_item_status_id = :kb_item_status_id, reference = :reference, "           \
  "api_reference = :api_reference, viewed_by_client_at = "                     \
  ":viewed_by_client_at,"                                                      \
  "updated_at = :updated_at, esr_id = :esr_id, qr_invoice_id = "               \
  ":qr_invoice_id,"                                                            \
  "template_slug = :template_slug, network_link = :network_link, "             \
  "_checksum = :_checksum, _last_updated = :_last_updated, total_gross = "     \
  ":total_gross, total_net = :total_net, total_taxes = :total_taxes, "         \
  "total_received_payments = :total_received_payments, total_credit_vouchers " \
  "= :total_credit_vouchers, total_remaining_payments = "                      \
  ":total_remaining_payments, total = :total, total_rounding_difference = "    \
  ":total_rounding_difference "                                                \
  "WHERE id = :id;"

void bx_object_invoice_free(void *data) {
  BXObjectInvoice *invoice = (BXObjectInvoice *)data;
  if (invoice == NULL) {
    return;
  }

  bx_object_free_value(&invoice->document_nr);
  bx_object_free_value(&invoice->is_valid_from);
  bx_object_free_value(&invoice->is_valid_to);
  bx_object_free_value(&invoice->contact_address);
  bx_object_free_value(&invoice->template_slug);
  bx_object_free_value(&invoice->updated_at);
  bx_object_free_value(&invoice->reference);
  bx_object_free_value(&invoice->api_reference);
  bx_object_free_value(&invoice->viewed_by_client_at);
  bx_object_free_value(&invoice->title);
  bx_object_free_value(&invoice->footer);
  bx_object_free_value(&invoice->header);
  bx_object_free_value(&invoice->network_link);

  free(invoice);
}

void bx_object_invoice_dump(void *data) {
  assert(data != NULL);

  BXObjectInvoice *invoice = (BXObjectInvoice *)data;

  _bx_dump_print_title("### DUMP '%s' ID:%lx CS:%lx ###",
                       invoice->document_nr.value, invoice->id,
                       invoice->checksum);
  _bx_dump_any("id", &invoice->id, 1);
  _bx_dump_any("user_id", &invoice->user_id, 1);
  _bx_dump_any("contact_id", &invoice->contact_id, 1);
  _bx_dump_any("contact_sub_id", &invoice->contact_sub_id, 1);
  _bx_dump_any("project_id", &invoice->project_id, 1);
  _bx_dump_any("bank_account_id", &invoice->bank_account_id, 1);
  _bx_dump_any("currency_id", &invoice->currency_id, 1);
  _bx_dump_any("payment_type_id", &invoice->payment_type_id, 1);
  _bx_dump_any("tva_id", &invoice->tva_type, 1);
  _bx_dump_any("kb_item_status_id", &invoice->kb_item_status_id, 1);
  _bx_dump_any("esr_id", &invoice->esr_id, 1);
  _bx_dump_any("qr_invoice_id", &invoice->qr_invoice_id, 1);

  _bx_dump_any("total_gross", &invoice->total_gross, 1);
  _bx_dump_any("total_net", &invoice->total_net, 1);
  _bx_dump_any("total_taxes", &invoice->total_taxes, 1);
  _bx_dump_any("total_reveived_payments", &invoice->total_received_payments, 1);
  _bx_dump_any("total_credit_vouchers", &invoice->total_credit_vouchers, 1);
  _bx_dump_any("total_remaining_payments", &invoice->total_remaining_payments,
               1);
  _bx_dump_any("total", &invoice->total, 1);
  _bx_dump_any("total_rounding_difference", &invoice->total_rounding_difference,
               1);

  _bx_dump_any("is_valid_from", &invoice->is_valid_from, 1);
  _bx_dump_any("is_valid_to", &invoice->is_valid_to, 1);
  _bx_dump_any("contact_address", &invoice->contact_address, 1);
  _bx_dump_any("template_slug", &invoice->template_slug, 1);
  _bx_dump_any("updated_at", &invoice->updated_at, 1);
  _bx_dump_any("reference", &invoice->reference, 1);
  _bx_dump_any("api_reference", &invoice->api_reference, 1);
  _bx_dump_any("viewed_by_client_at", &invoice->viewed_by_client_at, 1);
}

void *bx_object_invoice_decode(void *object) {
  json_t *jroot = (json_t *)object;
  BXObjectInvoice *invoice = NULL;
  XXH3_state_t *hashState = XXH3_createState();
  if (hashState == NULL) {
    return NULL;
  }
  XXH3_64bits_reset(hashState);
  invoice = calloc(1, sizeof(*invoice));
  if (invoice == NULL) {
    return NULL;
  }
  invoice->type = BXTypeInvoice;

  /* integer */
  invoice->id = bx_object_get_json_uint(jroot, "id", hashState);
  invoice->user_id = bx_object_get_json_uint(jroot, "user_id", hashState);
  invoice->contact_id = bx_object_get_json_uint(jroot, "contact_id", hashState);
  invoice->contact_sub_id =
      bx_object_get_json_uint(jroot, "contact_sub_id", hashState);
  invoice->project_id = bx_object_get_json_uint(jroot, "project_id", hashState);
  invoice->bank_account_id =
      bx_object_get_json_uint(jroot, "bank_account_id", hashState);
  invoice->currency_id =
      bx_object_get_json_uint(jroot, "currency_id", hashState);
  invoice->payment_type_id =
      bx_object_get_json_uint(jroot, "payment_type_id", hashState);
  invoice->tva_type = bx_object_get_json_uint(jroot, "mwst_type", hashState);
  invoice->kb_item_status_id =
      bx_object_get_json_uint(jroot, "kb_item_status_id", hashState);
  invoice->esr_id = bx_object_get_json_uint(jroot, "esr_id", hashState);
  invoice->qr_invoice_id =
      bx_object_get_json_uint(jroot, "qr_invoice_id", hashState);
  invoice->language_id =
      bx_object_get_json_uint(jroot, "language_id", hashState);
  invoice->mwst_type = bx_object_get_json_uint(jroot, "mwst_type", hashState);
  invoice->show_position_taxes =
      bx_object_get_json_uint(jroot, "show_position_taxes", hashState);
  invoice->mwst_is_net =
      bx_object_get_json_uint(jroot, "mwst_is_net", hashState);

  /* double */
  invoice->total_gross =
      bx_object_get_json_double(jroot, "total_gross", hashState);
  invoice->total_net = bx_object_get_json_double(jroot, "total_net", hashState);
  invoice->total_taxes =
      bx_object_get_json_double(jroot, "total_taxes", hashState);
  invoice->total_received_payments =
      bx_object_get_json_double(jroot, "total_received_payments", hashState);
  invoice->total_credit_vouchers =
      bx_object_get_json_double(jroot, "total_credit_vouchers", hashState);
  invoice->total_remaining_payments =
      bx_object_get_json_double(jroot, "total_remaining_payments", hashState);
  invoice->total = bx_object_get_json_double(jroot, "total", hashState);
  invoice->total_rounding_difference =
      bx_object_get_json_double(jroot, "total_rounding_difference", hashState);
  invoice->total_gross =
      bx_object_get_json_double(jroot, "total_gross", hashState);
  invoice->total_net = bx_object_get_json_double(jroot, "total_net", hashState);
  invoice->total_taxes =
      bx_object_get_json_double(jroot, "total_taxes", hashState);
  invoice->total_received_payments =
      bx_object_get_json_double(jroot, "total_received_payments", hashState);
  invoice->total_credit_vouchers =
      bx_object_get_json_double(jroot, "total_credit_vouchers", hashState);
  invoice->total_remaining_payments =
      bx_object_get_json_double(jroot, "total_remaining_payments", hashState);
  invoice->total = bx_object_get_json_double(jroot, "total", hashState);
  invoice->total_rounding_difference =
      bx_object_get_json_double(jroot, "total_rounding_difference", hashState);

  /* string */
  invoice->document_nr =
      bx_object_get_json_string(jroot, "document_nr", hashState);
  invoice->is_valid_from =
      bx_object_get_json_string(jroot, "is_valid_from", hashState);
  invoice->is_valid_to =
      bx_object_get_json_string(jroot, "is_valid_to", hashState);
  invoice->contact_address =
      bx_object_get_json_string(jroot, "contact_address", hashState);
  invoice->template_slug =
      bx_object_get_json_string(jroot, "template_slug", hashState);
  invoice->updated_at =
      bx_object_get_json_string(jroot, "updated_at", hashState);
  invoice->reference = bx_object_get_json_string(jroot, "reference", hashState);
  invoice->api_reference =
      bx_object_get_json_string(jroot, "api_reference", hashState);
  invoice->viewed_by_client_at =
      bx_object_get_json_string(jroot, "viewed_by_client_at", hashState);
  invoice->header = bx_object_get_json_string(jroot, "header", hashState);
  invoice->footer = bx_object_get_json_string(jroot, "footer", hashState);
  invoice->title = bx_object_get_json_string(jroot, "title", hashState);
  invoice->network_link =
      bx_object_get_json_string(jroot, "network_link", hashState);
  invoice->tva_is_net = bx_object_get_json_bool(jroot, "tva_is_net", hashState);

  invoice->checksum = XXH3_64bits_digest(hashState);

  XXH3_freeState(hashState);
  return invoice;
}

BXillError _bx_invoice_sync_item(MYSQL *conn, json_t *item, Cache *cache) {
  assert(item != NULL);
  BXillError RetVal = NoError;
  BXDatabaseQuery *query = NULL;
  BXObjectInvoice *invoice = NULL;

  invoice = bx_object_invoice_decode(item);
  if (invoice == NULL) {
    bx_log_debug("Decode failed");
    goto fail_and_return;
  }
  CacheState cacheItemState =
      cache_check_item(cache, (BXGeneric *)&invoice->id, invoice->checksum);
  /* 0 means in database with same hash */
  if (cacheItemState == CacheOk) {
    bx_object_invoice_free(invoice);
    return RetVal;
  }

  if (!bx_contact_is_in_database(conn, (BXGeneric *)&invoice->contact_id)) {
    goto fail_and_return;
  }
  if (!bx_project_is_in_database(conn, (BXGeneric *)&invoice->project_id) &&
      invoice->project_id.value > 0) {
    goto fail_and_return;
  } else if (invoice->project_id.value == 0) {
    invoice->project_id.isset = false;
  }

  /* 1: Not in database, so insert
   * 2: In database, different checksum, so update
   */
  if (cacheItemState == CacheNotSet) {
    query = bx_database_new_query(conn, QUERY_INSERT);
  } else if (cacheItemState == CacheNotSync) {
    query = bx_database_new_query(conn, QUERY_UPDATE);
  } else {
    goto fail_and_return;
  }
  if (query == NULL) {
    goto fail_and_return;
  }

  uint64_t now = time(NULL);

  bx_database_add_bxtype(query, ":id", (BXGeneric *)&invoice->id);
  bx_database_add_bxtype(query, ":document_nr",
                         (BXGeneric *)&invoice->document_nr);
  bx_database_add_bxtype(query, ":title", (BXGeneric *)&invoice->title);
  bx_database_add_bxtype(query, ":contact_id",
                         (BXGeneric *)&invoice->contact_id);
  bx_database_add_bxtype(query, ":contact_sub_id",
                         (BXGeneric *)&invoice->contact_sub_id);
  bx_database_add_bxtype(query, ":user_id", (BXGeneric *)&invoice->user_id);
  bx_database_add_bxtype(query, ":project_id",
                         (BXGeneric *)&invoice->project_id);
  bx_database_add_bxtype(query, ":language_id",
                         (BXGeneric *)&invoice->language_id);
  bx_database_add_bxtype(query, ":bank_account_id",
                         (BXGeneric *)&invoice->bank_account_id);
  bx_database_add_bxtype(query, ":currency_id",
                         (BXGeneric *)&invoice->currency_id);
  bx_database_add_bxtype(query, ":payment_type_id",
                         (BXGeneric *)&invoice->payment_type_id);
  bx_database_add_bxtype(query, ":header", (BXGeneric *)&invoice->header);
  bx_database_add_bxtype(query, ":footer", (BXGeneric *)&invoice->footer);
  bx_database_add_bxtype(query, ":mwst_type", (BXGeneric *)&invoice->mwst_type);
  bx_database_add_bxtype(query, ":mwst_is_net",
                         (BXGeneric *)&invoice->mwst_is_net);
  bx_database_add_bxtype(query, ":show_position_taxes",
                         (BXGeneric *)&invoice->show_position_taxes);
  bx_database_add_bxtype(query, ":is_valid_from",
                         (BXGeneric *)&invoice->is_valid_from);
  bx_database_add_bxtype(query, ":is_valid_to",
                         (BXGeneric *)&invoice->is_valid_to);
  bx_database_add_bxtype(query, ":contact_address",
                         (BXGeneric *)&invoice->contact_address);
  bx_database_add_bxtype(query, ":kb_item_status_id",
                         (BXGeneric *)&invoice->kb_item_status_id);
  bx_database_add_bxtype(query, ":reference", (BXGeneric *)&invoice->reference);
  bx_database_add_bxtype(query, ":api_reference",
                         (BXGeneric *)&invoice->api_reference);
  bx_database_add_bxtype(query, ":viewed_by_client_at",
                         (BXGeneric *)&invoice->viewed_by_client_at);
  bx_database_add_bxtype(query, ":updated_at",
                         (BXGeneric *)&invoice->updated_at);
  bx_database_add_bxtype(query, ":esr_id", (BXGeneric *)&invoice->esr_id);
  bx_database_add_bxtype(query, ":qr_invoice_id",
                         (BXGeneric *)&invoice->qr_invoice_id);
  bx_database_add_bxtype(query, ":template_slug",
                         (BXGeneric *)&invoice->template_slug);
  bx_database_add_bxtype(query, ":network_link",
                         (BXGeneric *)&invoice->network_link);

  bx_database_add_bxtype(query, ":total_gross",
                         (BXGeneric *)&invoice->total_gross);
  bx_database_add_bxtype(query, ":total_net", (BXGeneric *)&invoice->total_net);
  bx_database_add_bxtype(query, ":total_taxes",
                         (BXGeneric *)&invoice->total_taxes);
  bx_database_add_bxtype(query, ":total_received_payments",
                         (BXGeneric *)&invoice->total_received_payments);
  bx_database_add_bxtype(query, ":total_credit_vouchers",
                         (BXGeneric *)&invoice->total_credit_vouchers);
  bx_database_add_bxtype(query, ":total_remaining_payments",
                         (BXGeneric *)&invoice->total_remaining_payments);
  bx_database_add_bxtype(query, ":total", (BXGeneric *)&invoice->total);
  bx_database_add_bxtype(query, ":total_rounding_difference",
                         (BXGeneric *)&invoice->total_rounding_difference);

  bx_database_add_param_uint64(query, ":_checksum", &invoice->checksum);
  bx_database_add_param_uint64(query, ":_last_updated", &now);

  if (!bx_database_execute(query) || !bx_database_results(query)) {
    if (query->need_reconnect) {
      RetVal = ErrorSQLReconnect;
    } else {
      RetVal = ErrorGeneric;
    }
    goto fail_and_return;
  }

  if (query->warning_rows == 0 && !query->has_failed) {
    cache_set_item(cache, (BXGeneric *)&invoice->id, invoice->checksum);
  }
  bx_database_free_query(query);
  bx_object_invoice_free(invoice);
  query = NULL;

  return NoError;

fail_and_return:
  if (query != NULL) {
    bx_database_free_query(query);
  }
  if (invoice != NULL) {
    bx_object_invoice_free(invoice);
  }

  return RetVal;
}

#define GET_INVOICE_PATH "2.0/kb_invoice/$"
bool bx_invoice_sync_item(bXill *app, MYSQL *conn, BXGeneric *item,
                          Cache *cache) {
  assert(app != NULL);
  assert(item != NULL);
  BXNetRequest *request = NULL;
  request = bx_do_request(app->queue, NULL, GET_INVOICE_PATH, item);
  if (request == NULL) {
    return false;
  }
  if (request->response == NULL || request->response->http_code != 200) {
    if (request->response != NULL) {
      bx_log_debug("Code not good (response %p, code %d)", request->response,
                   request->response->http_code);
    } else {
      bx_log_debug("Response bad");
    }
    bx_net_request_free(request);
    return false;
  }
  bool retVal = _bx_invoice_sync_item(conn, request->decoded, cache);
  bx_net_request_free(request);
  return retVal;
}

#define WALK_INVOICE_PATH "2.0/kb_invoice?limit=$&offset=$"
BXillError bx_invoice_walk_items(bXill *app, MYSQL *conn, Cache *cache) {
  bx_log_debug("BX Walk Invoice Items");
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BX_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    arr_len = 0;
    BXNetRequest *request =
        bx_do_request(app->queue, NULL, WALK_INVOICE_PATH, &limit, &offset);
    if (request == NULL) {
      return ErrorNet;
    }
    if (!json_is_array(request->decoded)) {
      bx_net_request_free(request);
      return ErrorJSON;
    }
    arr_len = json_array_size(request->decoded);
    for (size_t i = 0; i < arr_len; i++) {
      BXillError e = _bx_invoice_sync_item(
          conn, json_array_get(request->decoded, i), cache);
      if (e != NoError) {
        bx_net_request_free(request);
        return e;
      }
    }
    bx_net_request_free(request);
    offset.value += limit.value;
    thrd_yield();
  } while (arr_len > 0);
  return NoError;
}
