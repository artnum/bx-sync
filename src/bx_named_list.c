#include "include/bx_named_list.h"
#include "include/bx_database.h"
#include "include/bx_object.h"
#include "include/bx_object_value.h"
#include "include/bx_utils.h"
#include <jansson.h>
#include <string.h>
#include <time.h>

static int table_ok(const char *table) {
  return table != NULL &&
         (strcmp(table, "unit") == 0 || strcmp(table, "salutation") == 0 ||
          strcmp(table, "title") == 0 || strcmp(table, "payment_type") == 0 ||
          strcmp(table, "contact_group") == 0 ||
          strcmp(table, "communication_kind") == 0 ||
          strcmp(table, "pr_project_state") == 0 ||
          strcmp(table, "pr_project_type") == 0 ||
          strcmp(table, "timesheet_status") == 0 ||
          strcmp(table, "todo_status") == 0 ||
          strcmp(table, "todo_priority") == 0 || strcmp(table, "stock") == 0 ||
          strcmp(table, "stock_place") == 0);
}

static BXillError upsert_named(MYSQL *conn, const char *table, BXUInteger *id,
                               BXString *name, uint64_t checksum) {
  char select_sql[160];
  char insert_sql[256];
  char update_sql[256];
  snprintf(select_sql, sizeof(select_sql),
           "SELECT _checksum FROM %s WHERE id = :id", table);
  snprintf(insert_sql, sizeof(insert_sql),
           "INSERT INTO %s (id, name, _checksum, _last_updated, _deleted) "
           "VALUES (:id, :name, :_checksum, :_last_updated, :_deleted)",
           table);
  snprintf(update_sql, sizeof(update_sql),
           "UPDATE %s SET name = :name, _checksum = :_checksum, "
           "_last_updated = :_last_updated, _deleted = :_deleted WHERE id = :id",
           table);

  BXDatabaseQuery *query = bx_database_new_query(conn, select_sql);
  if (query == NULL) {
    return ErrorGeneric;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)id);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }

  uint64_t now = (uint64_t)time(NULL);
  uint64_t not_deleted = 0;
  int need_insert = (query->results == NULL || query->results->column_count == 0);
  int same = 0;
  if (!need_insert &&
      (uint64_t)query->results->columns[0].i_value == checksum) {
    same = 1;
  }
  bx_database_free_query(query);
  if (same) {
    return NoError;
  }

  query = bx_database_new_query(conn, need_insert ? insert_sql : update_sql);
  if (query == NULL) {
    return ErrorGeneric;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)id);
  bx_database_add_bxtype(query, ":name", (BXGeneric *)name);
  bx_database_add_param_uint64(query, ":_checksum", &checksum);
  bx_database_add_param_uint64(query, ":_last_updated", &now);
  bx_database_add_param_uint64(query, ":_deleted", &not_deleted);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  bx_database_free_query(query);
  return NoError;
}

BXillError bx_named_list_walk(bXill *app, MYSQL *conn, const char *path_fmt,
                              const char *table) {
  if (app == NULL || conn == NULL || !table_ok(table) || path_fmt == NULL) {
    return ErrorGeneric;
  }
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BXILL_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    arr_len = 0;
    BXNetRequest *request =
        bx_do_request(app->queue, NULL, (char *)path_fmt, &limit, &offset);
    if (request == NULL) {
      return ErrorNet;
    }
    if (!json_is_array(request->decoded)) {
      bx_net_request_free(request);
      return ErrorJSON;
    }
    arr_len = json_array_size(request->decoded);
    for (size_t i = 0; i < arr_len; i++) {
      json_t *item = json_array_get(request->decoded, i);
      XXH3_state_t *hash = XXH3_createState();
      if (hash == NULL) {
        bx_net_request_free(request);
        return ErrorGeneric;
      }
      XXH3_64bits_reset(hash);
      BXUInteger id = bx_object_get_json_uint(item, "id", hash);
      BXString name = bx_object_get_json_string(item, "name", hash);
      uint64_t checksum = XXH3_64bits_digest(hash);
      XXH3_freeState(hash);
      BXillError e = upsert_named(conn, table, &id, &name, checksum);
      bx_object_free_value(&name);
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

BXillError bx_unit_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Unit Items");
  return bx_named_list_walk(app, conn, "2.0/unit?limit=$&offset=$", "unit");
}

BXillError bx_salutation_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Salutation Items");
  return bx_named_list_walk(app, conn, "2.0/salutation?limit=$&offset=$",
                            "salutation");
}

BXillError bx_title_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Title Items");
  return bx_named_list_walk(app, conn, "2.0/title?limit=$&offset=$", "title");
}

BXillError bx_payment_type_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Payment Type Items");
  return bx_named_list_walk(app, conn, "2.0/payment_type?limit=$&offset=$",
                            "payment_type");
}

BXillError bx_communication_kind_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/communication_kind?limit=$&offset=$",
                            "communication_kind");
}

BXillError bx_project_state_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/pr_project_state?limit=$&offset=$",
                            "pr_project_state");
}

BXillError bx_project_type_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/pr_project_type?limit=$&offset=$",
                            "pr_project_type");
}

BXillError bx_timesheet_status_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/timesheet_status?limit=$&offset=$",
                            "timesheet_status");
}

BXillError bx_todo_status_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/todo_status?limit=$&offset=$",
                            "todo_status");
}

BXillError bx_todo_priority_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/todo_priority?limit=$&offset=$",
                            "todo_priority");
}

BXillError bx_stock_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/stock?limit=$&offset=$", "stock");
}

BXillError bx_stock_place_walk_items(bXill *app, MYSQL *conn) {
  return bx_named_list_walk(app, conn, "2.0/stock_place?limit=$&offset=$",
                            "stock_place");
}
