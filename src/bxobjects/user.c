#include "../include/bxobjects/user.h"
#include "../include/bx_database.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"

#define GET_USER_PATH "3.0/users/$"
#define QUERY_INSERT                                                           \
  "INSERT INTO user (_checksum, _last_updated, "                               \
  "id, firstname, lastname, email, salutation_type, is_superadmin, "           \
  "is_accountant) "                                                            \
  "VALUES (:_checksum, :_last_updated, :id, :firstname, :lastname, "           \
  ":email, :salutation_type, :is_superadmin, :is_accountant);"
#define QUERY_UPDATE                                                           \
  "INSERT IGNORE INTO user (_checksum, _last_updated, "                        \
  "id, firstname, lastname, email, salutation_type, is_superadmin, "           \
  "is_accountant) "                                                            \
  "VALUES (:_checksum, :_last_updated, :id, :firstname, :lastname, "           \
  ":email, :salutation_type, :is_superadmin, :is_accountant);"

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
  XXH3_64bits_reset(hashState);
  user = calloc(1, sizeof(*user));
  if (user == NULL) {
    return NULL;
  }
  user->type = BXTypeContactGroup;
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

bool bx_user_sync_item(bXill *app, MYSQL *conn, BXGeneric *item) {
  bx_log_debug("BX Use Sync Item");
  BXNetRequest *request = bx_do_request(app->queue, NULL, GET_USER_PATH, item);
  if (request == NULL) {
    return false;
  }
  if (request == NULL || request->response == NULL ||
      request->response->http_code != 200) {
    return false;
  }
  BXObjectUser *user = decode_object(request->decoded);
  bx_net_request_free(request);
  if (user == NULL) {
    return false;
  }

  char is_superadmin = user->remote_is_superadmin.value ? 1 : 0;
  char is_accountant = user->remote_is_accountant.value ? 1 : 0;
  time_t now = time(NULL);
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT _checksum FROM user WHERE id = :id;");
  bx_database_add_param_int64(query, ":id", &user->remote_id.value);
  bx_database_execute(query);
  bx_database_results(query);
  if (query->results == NULL || query->results->column_count == 0) {
    bx_database_free_query(query);
    query = bx_database_new_query(conn, QUERY_INSERT);

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

    bx_database_execute(query);
    bx_database_free_query(query);
    free_object(user);
    return true;
  }
  if (query->results[0].columns[0].i_value == user->checksum) {
    bx_database_free_query(query);
    free_object(user);
    return true;
  }

  bx_database_free_query(query);
  query = bx_database_new_query(conn, QUERY_UPDATE);
  uint64_t not_deleted = 0;
  bx_database_add_param_char(query, ":firstname", user->remote_firstname.value,
                             user->remote_firstname.value_len);
  bx_database_add_param_char(query, ":lastname", user->remote_lastname.value,
                             user->remote_lastname.value_len);
  bx_database_add_param_char(query, ":email", user->remote_email.value,
                             user->remote_email.value_len);
  bx_database_add_param_char(query, ":salutation_type",
                             user->remote_salutation_type.value,
                             user->remote_salutation_type.value_len);

  bx_database_add_param_uint8(query, ":is_superadmin", &is_superadmin);
  bx_database_add_param_uint8(query, ":is_accountant", &is_accountant);

  bx_database_add_param_uint64(query, ":_checksum", &user->checksum);
  bx_database_add_param_uint64(query, ":_last_updated", &now);
  bx_database_add_param_uint64(query, ":_deleted", &not_deleted);

  bx_database_execute(query);
  bx_database_free_query(query);
  free_object(user);
  return true;
}
