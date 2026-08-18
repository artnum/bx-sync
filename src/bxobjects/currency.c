#include "../include/bxobjects/currency.h"
#include "../include/bx_database.h"
#include "../include/bx_object.h"
#include "../include/bx_utils.h"
#include <jansson.h>
#include <time.h>

#define QUERY_INSERT                                                           \
  "INSERT INTO currency (id, name, round_factor, exchange_rate, "              \
  "exchange_rate_id, ratio, source, source_reason, exchange_rate_date, "       \
  "_checksum, _last_updated, _deleted) VALUES (:id, :name, :round_factor, "    \
  ":exchange_rate, :exchange_rate_id, :ratio, :source, :source_reason, "       \
  ":exchange_rate_date, :_checksum, :_last_updated, :_deleted)"
#define QUERY_UPDATE                                                           \
  "UPDATE currency SET name = :name, round_factor = :round_factor, "           \
  "exchange_rate = :exchange_rate, exchange_rate_id = :exchange_rate_id, "      \
  "ratio = :ratio, source = :source, source_reason = :source_reason, "         \
  "exchange_rate_date = :exchange_rate_date, _checksum = :_checksum, "          \
  "_last_updated = :_last_updated, _deleted = :_deleted WHERE id = :id"

static void free_content(BXObjectCurrency *c) {
  bx_object_free_value(&c->name);
  bx_object_free_value(&c->source);
  bx_object_free_value(&c->source_reason);
  bx_object_free_value(&c->exchange_rate_date);
}

static BXObjectCurrency *decode_object(json_t *object) {
  BXObjectCurrency *currency = calloc(1, sizeof(*currency));
  if (currency == NULL) {
    return NULL;
  }
  XXH3_state_t *hash_state = XXH3_createState();
  if (hash_state == NULL) {
    free(currency);
    return NULL;
  }
  XXH3_64bits_reset(hash_state);
  currency->type = BXTypeCurrency;
  bxo_getuint(currency, id);
  bxo_getstr(currency, name);
  bxo_getdouble(currency, round_factor);
  bxo_getdouble(currency, exchange_rate);
  bxo_getuint(currency, exchange_rate_id);
  bxo_getdouble(currency, ratio);
  bxo_getstr(currency, source);
  bxo_getstr(currency, source_reason);
  bxo_getstr(currency, exchange_rate_date);
  bxo_checksum(currency);
  return currency;
}

static BXillError persist(MYSQL *conn, BXObjectCurrency *currency) {
  BXDatabaseQuery *query = bx_database_new_query(
      conn, "SELECT _checksum FROM currency WHERE id = :id");
  if (query == NULL) {
    return ErrorGeneric;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)&currency->id);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  int need_insert =
      (query->results == NULL || query->results->column_count == 0);
  if (!need_insert &&
      (uint64_t)query->results->columns[0].i_value == currency->checksum) {
    bx_database_free_query(query);
    return NoError;
  }
  bx_database_free_query(query);

  query = bx_database_new_query(conn, need_insert ? QUERY_INSERT : QUERY_UPDATE);
  if (query == NULL) {
    return ErrorGeneric;
  }
  uint64_t now = (uint64_t)time(NULL);
  uint64_t not_deleted = 0;
  if (!bxd_bind(currency, id) || !bxd_bind(currency, name) ||
      !bxd_bind(currency, round_factor) || !bxd_bind(currency, exchange_rate) ||
      !bxd_bind(currency, exchange_rate_id) || !bxd_bind(currency, ratio) ||
      !bxd_bind(currency, source) || !bxd_bind(currency, source_reason) ||
      !bxd_bind(currency, exchange_rate_date) ||
      !bx_database_add_param_uint64(query, ":_checksum", &currency->checksum) ||
      !bx_database_add_param_uint64(query, ":_last_updated", &now) ||
      !bx_database_add_param_uint64(query, ":_deleted", &not_deleted)) {
    bx_database_free_query(query);
    return ErrorGeneric;
  }
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  bx_database_free_query(query);
  return NoError;
}

BXillError bx_currency_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Currency Items");
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BXILL_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    BXNetRequest *request = bx_do_request(
        app->queue, NULL, "3.0/currencies?limit=$&offset=$", &limit, &offset);
    if (request == NULL) {
      return ErrorNet;
    }
    if (!json_is_array(request->decoded)) {
      bx_net_request_free(request);
      return ErrorJSON;
    }
    arr_len = json_array_size(request->decoded);
    for (size_t i = 0; i < arr_len; i++) {
      BXObjectCurrency *currency =
          decode_object(json_array_get(request->decoded, i));
      if (currency == NULL) {
        bx_net_request_free(request);
        return ErrorGeneric;
      }
      BXillError e = persist(conn, currency);
      free_content(currency);
      free(currency);
      if (e != NoError) {
        bx_net_request_free(request);
        return e;
      }
    }
    bx_net_request_free(request);
    offset.value += limit.value;
  } while (arr_len > 0);
  return NoError;
}
