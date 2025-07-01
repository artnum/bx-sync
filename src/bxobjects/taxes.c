#include "../include/bxobjects/taxes.h"
#include "../include/bx_database.h"
#include "../include/bx_object.h"
#include "../include/bx_utils.h"
#include <assert.h>
#include <jansson.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define QUERY_INSERT                                                           \
  "INSERT IGNORE INTO taxes (`id`, `uuid`, `name`, `code`, `digit`, `type`, "  \
  "`account_id`, "                                                             \
  "`tax_settlement_type`, `value`, `net_tax_value`, `start_year`, "            \
  "`end_year`, "                                                               \
  "`is_active`, `display_name`, `start_month`, `end_month`, `_checksum`, "     \
  "`_last_updated`) VALUES (:id, :uuid, :name, :code, :digit, :taxtype, "      \
  ":account_id, :tax_settlement_type, :value, :net_tax_value, :start_year, "   \
  ":end_year, :is_active, :display_name, :start_month, :end_month, "           \
  ":_checksum, :_last_updated);"

void *bx_object_tax_decode(void *jroot) {
  assert(jroot != NULL);
  BXObjectTax *tax = NULL;
  XXH3_state_t *hashState = XXH3_createState();
  if (hashState == NULL) {
    return NULL;
  }
  XXH3_64bits_reset(hashState);
  tax = calloc(1, sizeof(*tax));
  if (tax == NULL) {
    return NULL;
  }
  tax->type = BXTypeInvoiceTax;
  tax->id = bx_object_get_json_uint(jroot, "id", hashState);
  tax->uuid = bx_object_get_json_uuid(jroot, "uuid", hashState);
  tax->name = bx_object_get_json_string(jroot, "name", hashState);
  tax->digit = bx_object_get_json_int(jroot, "digit", hashState);
  tax->taxtype = bx_object_get_json_string(jroot, "type", hashState);
  tax->account_id = bx_object_get_json_uint(jroot, "account_id", hashState);
  tax->tax_settlement_type =
      bx_object_get_json_string(jroot, "tax_settlement_type", hashState);
  tax->value = bx_object_get_json_double(jroot, "value", hashState);
  tax->net_tax_value =
      bx_object_get_json_string(jroot, "net_tax_value", hashState);
  tax->start_year = bx_object_get_json_int(jroot, "start_year", hashState);
  tax->end_year = bx_object_get_json_int(jroot, "end_year", hashState);
  tax->start_month = bx_object_get_json_int(jroot, "start_month", hashState);
  tax->end_month = bx_object_get_json_int(jroot, "end_month", hashState);
  tax->display_name =
      bx_object_get_json_string(jroot, "display_name", hashState);
  tax->is_active = bx_object_get_json_bool(jroot, "is_active", hashState);

  tax->checksum = XXH3_64bits_digest(hashState);
  XXH3_freeState(hashState);

  return tax;
}

void bx_object_tax_free(void *data) {
  BXObjectTax *tax = (BXObjectTax *)data;

  if (tax == NULL) {
    return;
  }
  bx_object_free_value(&tax->id);
  bx_object_free_value(&tax->uuid);
  bx_object_free_value(&tax->name);
  bx_object_free_value(&tax->digit);
  bx_object_free_value(&tax->taxtype);
  bx_object_free_value(&tax->account_id);
  bx_object_free_value(&tax->tax_settlement_type);
  bx_object_free_value(&tax->value);
  bx_object_free_value(&tax->net_tax_value);
  bx_object_free_value(&tax->start_year);
  bx_object_free_value(&tax->start_month);
  bx_object_free_value(&tax->end_year);
  bx_object_free_value(&tax->end_month);
  bx_object_free_value(&tax->display_name);
  bx_object_free_value(&tax->is_active);
  free(tax);
}

void bx_dump(BXObjectTax *tax) {
  _bx_dump_print_title("Taxes %ld", tax->id.value);
  _bx_dump_any("id", &tax->id, 0);
  _bx_dump_any("uuid", &tax->uuid, 0);
  _bx_dump_any("name", &tax->name, 0);
  _bx_dump_any("type", &tax->taxtype, 0);
  _bx_dump_any("account_id", &tax->account_id, 0);
  _bx_dump_any("tax_settlement_type", &tax->tax_settlement_type, 0);
  _bx_dump_any("value", &tax->value, 0);
  _bx_dump_any("net_tax_value", &tax->net_tax_value, 0);
  _bx_dump_any("start_year", &tax->start_year, 0);
  _bx_dump_any("end_year", &tax->end_year, 0);
  _bx_dump_any("display_name", &tax->display_name, 0);
  _bx_dump_any("is_active", &tax->is_active, 0);
  _bx_dump_any("start_month", &tax->start_month, 0);
  _bx_dump_any("end_month", &tax->end_month, 0);
}

BXillError _bx_insert_tax(MYSQL *conn, BXObjectTax *tax) {
  BXDatabaseQuery *query = bx_database_new_query(conn, QUERY_INSERT);
  if (!query) {
    return false;
  }

  time_t now;
  time(&now);
  if (!bxd_bind(tax, id) || !bxd_bind(tax, uuid) || !bxd_bind(tax, name) ||
      !bxd_bind(tax, digit) || !bxd_bind(tax, taxtype) ||
      !bxd_bind(tax, account_id) || !bxd_bind(tax, tax_settlement_type) ||
      !bxd_bind(tax, value) || !bxd_bind(tax, net_tax_value) ||
      !bxd_bind(tax, start_year) || !bxd_bind(tax, start_month) ||
      !bxd_bind(tax, end_year) || !bxd_bind(tax, end_month) ||
      !bxd_bind(tax, display_name) || !bxd_bind(tax, is_active) ||
      !bx_database_add_param_uint64(query, ":_checksum", &tax->checksum) ||
      !bx_database_add_param_uint64(query, ":_last_updated", &now)) {
    bx_database_free_query(query);
    return ErrorGeneric;
  }

  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = ErrorGeneric;
    if (query->need_reconnect) {
      e = ErrorSQLReconnect;
    }
    bx_database_free_query(query);
    return e;
  }
  bx_database_free_query(query);
  return NoError;
}

ObjectState bx_taxes_check_database(MYSQL *conn, BXObjectTax *tax) {
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT _checksum FROM taxes WHERE id = :id");
  if (query == NULL) {
    return Error;
  }
  if (!bx_database_add_bxtype(query, ":id", (BXGeneric *)&tax->id) ||
      !bx_database_execute(query) || !bx_database_results(query)) {
    bx_database_free_query(query);
    return Error;
  }
  if (query->results == NULL || query->results->column_count == 0) {
    bx_database_free_query(query);
    return NeedCreate;
  }
  if (query->results->columns[0].i_value != tax->checksum) {
    bx_database_free_query(query);
    return NeedUpdate;
  }
  bx_database_free_query(query);
  return NeedNothing;
}

BXillError _bx_sync_item(MYSQL *conn, json_t *item) {
  BXObjectTax *tax = NULL;
  tax = bx_object_tax_decode(item);
  if (!tax) {
    return false;
  }
  BXillError RetVal = NoError;
  switch (bx_taxes_check_database(conn, tax)) {
  case Error:
    bx_log_error("SQL Failed check tax %ld", tax->id.value);
    RetVal = ErrorGeneric;
    break;
  case NeedCreate:
    RetVal = _bx_insert_tax(conn, tax);
    break;
  case NeedUpdate:
    bx_log_info("UPDATE not yet implemented for %ld", tax->id.value);
    break;
  case NeedNothing:
    break;
  }
  bx_object_tax_free(tax);
  return RetVal;
}

const struct timespec TAXES_SLEEP = {.tv_nsec = 0, .tv_sec = 5};
#define WALK_TAXES_PATH "3.0/taxes?limit=$&offset=$&scope=$"
BXillError bx_taxes_walk_item(bXill *app, MYSQL *conn) {
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BX_LIST_LIMIT};
  BXString scope = {.type = BX_OBJECT_TYPE_STRING,
                    .isset = true,
                    .value = "active",
                    .value_len = sizeof("active")};

  for (int i = 0; i < 2; i++) {
    size_t arr_len = 0;
    do {
      arr_len = 0;
      BXNetRequest *request = bx_do_request(app->queue, NULL, WALK_TAXES_PATH,
                                            &limit, &offset, &scope);
      if (request == NULL) {
        bx_log_debug("Failed request allocation");
        return ErrorNet;
      }
      bx_log_debug("WALK TAXES %s", request->response->data);
      if (!json_is_array(request->decoded)) {
        bx_net_request_free(request);
        return ErrorJSON;
      }
      arr_len = json_array_size(request->decoded);
      for (size_t j = 0; j < arr_len; j++) {
        BXillError e = _bx_sync_item(conn, json_array_get(request->decoded, j));
        if (e != NoError) {
          bx_net_request_free(request);
          return e;
        }
      }
      bx_net_request_free(request);
      offset.value += limit.value;
      thrd_sleep(&TAXES_SLEEP, NULL);
    } while (arr_len > 0);
    scope.value = "inactive";
    scope.value_len = sizeof("inactive");
  }
  return NoError;
}
