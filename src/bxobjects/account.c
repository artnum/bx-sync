#include "../include/bxobjects/account.h"
#include "../include/bx_database.h"
#include "../include/bx_object.h"
#include "../include/bx_utils.h"
#include <jansson.h>
#include <time.h>

#define QUERY_INSERT                                                           \
  "INSERT INTO account (id, uuid, account_no, name, account_type, tax_id, "    \
  "fibu_account_group_id, is_active, is_locked, _checksum, _last_updated, "    \
  "_deleted) VALUES (:id, :uuid, :account_no, :name, :account_type, :tax_id, " \
  ":fibu_account_group_id, :is_active, :is_locked, :_checksum, "               \
  ":_last_updated, :_deleted)"
#define QUERY_UPDATE                                                           \
  "UPDATE account SET uuid = :uuid, account_no = :account_no, name = :name, "  \
  "account_type = :account_type, tax_id = :tax_id, "                           \
  "fibu_account_group_id = :fibu_account_group_id, is_active = :is_active, "   \
  "is_locked = :is_locked, _checksum = :_checksum, "                           \
  "_last_updated = :_last_updated, _deleted = :_deleted WHERE id = :id"

static void free_content(BXObjectAccount *a) {
  bx_object_free_value(&a->account_no);
  bx_object_free_value(&a->name);
}

static BXObjectAccount *decode_object(json_t *object) {
  BXObjectAccount *account = calloc(1, sizeof(*account));
  if (account == NULL) {
    return NULL;
  }
  XXH3_state_t *hash_state = XXH3_createState();
  if (hash_state == NULL) {
    free(account);
    return NULL;
  }
  XXH3_64bits_reset(hash_state);
  account->type = BXTypeAccount;
  bxo_getuint(account, id);
  bxo_getuuid(account, uuid);
  bxo_getstr(account, account_no);
  bxo_getstr(account, name);
  bxo_getint(account, account_type);
  bxo_getuint(account, tax_id);
  bxo_getuint(account, fibu_account_group_id);
  bxo_getbool(account, is_active);
  bxo_getbool(account, is_locked);
  bxo_checksum(account);
  return account;
}

static BXillError persist(MYSQL *conn, BXObjectAccount *account) {
  BXDatabaseQuery *query = bx_database_new_query(
      conn, "SELECT _checksum FROM account WHERE id = :id");
  if (query == NULL) {
    return ErrorGeneric;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)&account->id);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  int need_insert =
      (query->results == NULL || query->results->column_count == 0);
  if (!need_insert &&
      (uint64_t)query->results->columns[0].i_value == account->checksum) {
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
  if (!bxd_bind(account, id) || !bxd_bind(account, uuid) ||
      !bxd_bind(account, account_no) || !bxd_bind(account, name) ||
      !bxd_bind(account, account_type) || !bxd_bind(account, tax_id) ||
      !bxd_bind(account, fibu_account_group_id) ||
      !bxd_bind(account, is_active) || !bxd_bind(account, is_locked) ||
      !bx_database_add_param_uint64(query, ":_checksum", &account->checksum) ||
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

BXillError bx_account_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Account Items");
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BXILL_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    BXNetRequest *request = bx_do_request(
        app->queue, NULL, "2.0/accounts?limit=$&offset=$", &limit, &offset);
    if (request == NULL) {
      return ErrorNet;
    }
    if (!json_is_array(request->decoded)) {
      bx_net_request_free(request);
      return ErrorJSON;
    }
    arr_len = json_array_size(request->decoded);
    for (size_t i = 0; i < arr_len; i++) {
      BXObjectAccount *account =
          decode_object(json_array_get(request->decoded, i));
      if (account == NULL) {
        bx_net_request_free(request);
        return ErrorGeneric;
      }
      BXillError e = persist(conn, account);
      free_content(account);
      free(account);
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
