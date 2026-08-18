#include "../include/bxobjects/user.h"
#include "../include/bx_database.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"
#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GET_USER_PATH "3.0/users/$"
#define QUERY_INSERT                                                          \
  "INSERT IGNORE INTO user ("                                                 \
    "_checksum, "                                                             \
    "_last_updated, "                                                         \
    "_deleted, "                                                              \
    "id, "                                                                    \
    "firstname, "                                                             \
    "lastname, "                                                              \
    "email, "                                                                 \
    "salutation_type, "                                                       \
    "is_superadmin, "                                                         \
    "is_accountant) "                                                         \
  "VALUES ("                                                                  \
    ":_checksum, "                                                            \
    ":_last_updated, "                                                        \
    ":_deleted , "                                                            \
    ":id, "                                                                   \
    ":firstname, "                                                            \
    ":lastname, "                                                             \
    ":email, "                                                                \
    ":salutation_type, "                                                      \
    ":is_superadmin, "                                                        \
    ":is_accountant);"
#define QUERY_UPDATE                                                          \
  "UPDATE user SET "                                                          \
    "_checksum = :_checksum, "                                                \
    "_last_updated = :_last_updated, "                                        \
    "_deleted = :_deleted, "                                                  \
    "lastname = :lastname, "                                                  \
    "firstname = :firstname, "                                                \
    "email = :email, "                                                        \
    "salutation_type = :salutation_type, "                                    \
    "is_superadmin = :is_superadmin, "                                        \
    "is_accountant = :is_accountant "                                         \
  "WHERE id = :id;"

static inline void free_object(BXObjectUser *user) {
  if (user == NULL) {
    return;
  }
  bx_object_free_value(&user->remote_email);
  bx_object_free_value(&user->remote_firstname);
  bx_object_free_value(&user->remote_id);
  bx_object_free_value(&user->remote_lastname);
  bx_object_free_value(&user->remote_is_accountant);
  bx_object_free_value(&user->remote_is_superadmin);
  bx_object_free_value(&user->remote_salutation_type);
  free(user);
}

static inline BXObjectUser *decode_object(json_t *root) {
  json_t *object = (json_t *)root;
  BXObjectUser *user = NULL;
  XXH3_state_t *hashState = XXH3_createState();
  if (hashState == NULL) {
    return NULL;
  }
  user = calloc(1, sizeof(*user));
  if (user == NULL) {
    XXH3_freeState(hashState);
    return NULL;
  }
  
  XXH3_64bits_reset(hashState);
  user->type = BXTypeUser;
  user->remote_id = bx_object_get_json_uint(object, "id", hashState);
  user->remote_firstname =
      bx_object_get_json_string(object, "firstname", hashState);
  user->remote_lastname =
      bx_object_get_json_string(object, "lastname", hashState);
  user->remote_email = bx_object_get_json_string(object, "email", hashState);
  user->remote_salutation_type =
      bx_object_get_json_string(object, "salutation_type", hashState);
  user->remote_is_accountant =
      bx_object_get_json_bool(object, "is_accountant", hashState);
  user->remote_is_superadmin =
      bx_object_get_json_bool(object, "is_superadmin", hashState);

  user->checksum = XXH3_64bits_digest(hashState);
  XXH3_freeState(hashState);

  return user;
}

bool bx_user_is_in_database(MYSQL *conn, BXGeneric *item) {
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT id FROM user  WHERE id = :id;");
  if (query == NULL) {
    return false;
  }
  bx_database_add_bxtype(query, ":id", item);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    bx_database_free_query(query);
    return false;
  }

  if (query->results == NULL || query->results->column_count == 0) {
    bx_database_free_query(query);
    return false;
  }

  bx_database_free_query(query);
  return true;
}

static void ensure_str(BXString *s) {
  if (s->isset && s->value != NULL) {
    return;
  }
  s->type = BX_OBJECT_TYPE_STRING;
  s->isset = true;
  s->value = calloc(1, 1);
  s->value_len = 0;
}

static bool persist_user(MYSQL *conn, BXObjectUser *user) {
  if (conn == NULL || user == NULL || !user->remote_id.isset) {
    return false;
  }
  ensure_str(&user->remote_firstname);
  ensure_str(&user->remote_lastname);
  ensure_str(&user->remote_email);
  ensure_str(&user->remote_salutation_type);

  char is_superadmin = user->remote_is_superadmin.value ? 1 : 0;
  char is_accountant = user->remote_is_accountant.value ? 1 : 0;
  time_t now = time(NULL);
  uint64_t not_deleted = 0;
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT _checksum FROM user WHERE id = :id;");
  if (query == NULL) {
    return false;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)&user->remote_id);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    bx_database_free_query(query);
    return false;
  }
  int need_insert =
      (query->results == NULL || query->results->column_count == 0);
  if (!need_insert &&
      (uint64_t)query->results->columns[0].i_value == user->checksum) {
    bx_database_free_query(query);
    return true;
  }
  bx_database_free_query(query);

  query = bx_database_new_query(conn, need_insert ? QUERY_INSERT : QUERY_UPDATE);
  if (query == NULL) {
    return false;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)&user->remote_id);
  bx_database_add_bxtype(query, ":firstname",
                         (BXGeneric *)&user->remote_firstname);
  bx_database_add_bxtype(query, ":lastname",
                         (BXGeneric *)&user->remote_lastname);
  bx_database_add_bxtype(query, ":email", (BXGeneric *)&user->remote_email);
  bx_database_add_bxtype(query, ":salutation_type",
                         (BXGeneric *)&user->remote_salutation_type);
  bx_database_add_param_uint8(query, ":is_superadmin", &is_superadmin);
  bx_database_add_param_uint8(query, ":is_accountant", &is_accountant);
  bx_database_add_param_uint64(query, ":_checksum", &user->checksum);
  bx_database_add_param_uint64(query, ":_last_updated", &now);
  bx_database_add_param_uint64(query, ":_deleted", &not_deleted);
  bool ok = bx_database_execute(query) && bx_database_results(query) &&
            bx_database_persist_ok(query);
  bx_database_free_query(query);
  return ok;
}

static BXObjectUser *decode_user_json(json_t *root) {
  if (json_is_object(root)) {
    json_t *data = json_object_get(root, "data");
    if (json_is_object(data)) {
      root = data;
    }
  }
  if (!json_is_object(root)) {
    return NULL;
  }
  return decode_object(root);
}

bool bx_user_sync_item(bXill *app, MYSQL *conn, BXGeneric *item) {
  bx_log_debug("BX Use Sync Item");
  const char *paths[] = {GET_USER_PATH, "3.0/fictional_users/$", NULL};
  for (int i = 0; paths[i] != NULL; i++) {
    BXNetRequest *request =
        bx_do_request(app->queue, NULL, (char *)paths[i], item);
    if (request == NULL || request->response == NULL ||
        request->response->http_code != 200) {
      bx_net_request_free(request);
      continue;
    }
    BXObjectUser *user = decode_user_json(request->decoded);
    bx_net_request_free(request);
    if (user == NULL) {
      continue;
    }
    bool ok = persist_user(conn, user);
    free_object(user);
    if (ok) {
      return true;
    }
  }
  return false;
}

static BXillError walk_user_path(bXill *app, MYSQL *conn, const char *path) {
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BXILL_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    BXNetRequest *request =
        bx_do_request(app->queue, NULL, (char *)path, &limit, &offset);
    if (request == NULL) {
      return ErrorNet;
    }
    json_t *arr = request->decoded;
    if (json_is_object(arr)) {
      json_t *data = json_object_get(arr, "data");
      if (json_is_array(data)) {
        arr = data;
      }
    }
    if (!json_is_array(arr)) {
      bx_net_request_free(request);
      return ErrorJSON;
    }
    arr_len = json_array_size(arr);
    for (size_t i = 0; i < arr_len; i++) {
      BXObjectUser *user = decode_user_json(json_array_get(arr, i));
      if (user == NULL) {
        continue;
      }
      bool ok = persist_user(conn, user);
      free_object(user);
      if (!ok) {
        bx_log_error("Failed persist user from %s", path);
      }
    }
    bx_net_request_free(request);
    offset.value += limit.value;
  } while (arr_len > 0);
  return NoError;
}

BXillError bx_user_walk_items(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk User Items");
  BXillError e = walk_user_path(app, conn, "3.0/users?limit=$&offset=$");
  if (e == ErrorSQLReconnect) {
    return e;
  }
  if (e != NoError) {
    bx_log_error("User list walk failed: %d", (int)e);
  }
  e = walk_user_path(app, conn, "3.0/fictional_users?limit=$&offset=$");
  if (e != NoError && e != ErrorSQLReconnect) {
    bx_log_error("Fictional user list walk failed: %d", (int)e);
    return NoError;
  }
  return e;
}
