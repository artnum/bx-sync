#include "../include/bxobjects/project.h"
#include "../include/bx_database.h"
#include "../include/bx_net.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"
#include <jansson.h>
#include <stdint.h>
#include <sys/types.h>
#include <threads.h>

#define GET_PROJECT_PATH "2.0/pr_project/$"
#define QUERY_UPDATE                                                           \
  "UPDATE pr_project SET uuid = :uuid, nr = :nr, name = :name,"                \
  "start_date = :start_date, end_date = :end_date, comment = :comment,"        \
  "pr_state_id = :pr_state_id, pr_project_type_id = :pr_project_type_id,"      \
  "contact_id = :contact_id, contact_sub_id = :contact_sub_id,"                \
  "pr_invoice_type_id = :pr_invoice_type_id, pr_invoice_type_amount = "        \
  ":pr_invoice_type_amount"                                                    \
  "pr_budget_type_id = :pr_budget_type_id, pr_budget_type_amount = "           \
  ":pr_budget_type_amount"                                                     \
  "_checksum := :_checkum, _last_updated = :_last_updated WHERE id = :id;"
#define QUERY_INSERT                                                           \
  "INSERT IGNORE INTO pr_project (id, uuid, nr, name, start_date,"             \
  "end_date, comment, pr_state_id, pr_project_type_id, contact_id,"            \
  "contact_sub_id, pr_invoice_type_id, pr_invoice_type_amount, "               \
  "pr_budget_type_id,"                                                         \
  "pr_budget_type_amount, _checksum, _last_updated"                            \
  ") VALUES ("                                                                 \
  ":id, :uuid, :nr, :name, :start_date,"                                       \
  ":end_date, :comment, :pr_state_id, :pr_project_type_id, :contact_id,"       \
  ":contact_sub_id, :pr_invoice_type_id, :pr_invoice_type_amount, "            \
  ":pr_budget_type_id,"                                                        \
  ":pr_budget_type_amount, :_checksum, :_last_updated"                         \
  ");"

static BXObjectProject *decode_object(json_t *object) {
  BXObjectProject *project = NULL;
  project = calloc(1, sizeof(*project));
  if (project == NULL) {
    return NULL;
  }
  XXH3_state_t *hash_state = XXH3_createState();
  if (hash_state == NULL) {
    return NULL;
  }
  XXH3_64bits_reset(hash_state);
  project->type = BXTypeProject;

  bxo_getuint(project, id);
  bxo_getuint(project, pr_state_id);
  bxo_getuint(project, pr_project_type_id);
  bxo_getuint(project, contact_id);
  bxo_getuint(project, contact_sub_id);
  bxo_getuint(project, pr_invoice_type_id);
  bxo_getuint(project, pr_budget_type_id);

  bxo_getuuid(project, uuid);

  bxo_getstr(project, nr);
  bxo_getstr(project, name);
  bxo_getstr(project, start_date);
  bxo_getstr(project, end_date);
  bxo_getstr(project, comment);

  bxo_getdouble(project, pr_invoice_type_amount);
  bxo_getdouble(project, pr_budget_type_amount);

  bxo_checksum(project);

  return project;
}

static void free_content(BXObjectProject *project) {
  bx_object_free_value(&project->nr);
  bx_object_free_value(&project->name);
  bx_object_free_value(&project->start_date);
  bx_object_free_value(&project->end_date);
  bx_object_free_value(&project->comment);
}

void bx_project_free(BXObjectProject *project) {
  free_content(project);
  free(project);
}

void bx_project_dump(BXObjectProject *project) {
  printf("\n");
  _bx_dump_any("id", &project->id, 0);
  _bx_dump_any("uuid", &project->uuid, 0);
  _bx_dump_any("nr", &project->nr, 0);
  _bx_dump_any("name", &project->name, 0);
  _bx_dump_any("start_date", &project->start_date, 0);
  _bx_dump_any("end_date", &project->end_date, 0);
  _bx_dump_any("comment", &project->comment, 0);
  _bx_dump_any("pr_state_id", &project->pr_state_id, 0);
  _bx_dump_any("pr_project_type_id", &project->pr_project_type_id, 0);
  _bx_dump_any("contact_id", &project->contact_id, 0);
  _bx_dump_any("contact_sub_id", &project->contact_sub_id, 0);
  _bx_dump_any("pr_invoice_type_id", &project->pr_project_type_id, 0);
  _bx_dump_any("pr_invoice_type_amount", &project->pr_invoice_type_amount, 0);
  _bx_dump_any("pr_budget_type_id", &project->pr_budget_type_id, 0);
  _bx_dump_any("pr_budget_type_amount", &project->pr_budget_type_amount, 0);
}

ObjectState bx_project_check_database(MYSQL *conn, BXObjectProject *project) {
  BXDatabaseQuery *query = bx_database_new_query(
      conn, "SELECT _checksum FROM pr_project WHERE id = :id");
  if (query == NULL) {
    return Error;
  }
  if (!bx_database_add_bxtype(query, ":id", (BXGeneric *)&project->id) ||
      !bx_database_execute(query) || !bx_database_results(query)) {
    bx_database_free_query(query);
    return Error;
  }
  if (query->results == NULL || query->results->column_count == 0) {
    bx_database_free_query(query);
    return NeedCreate;
  }
  if (query->results->columns[0].i_value != project->checksum) {
    bx_database_free_query(query);
    return NeedUpdate;
  }
  bx_database_free_query(query);
  return NeedNothing;
}

bool bx_project_is_in_database(MYSQL *conn, BXGeneric *item) {
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT id FROM pr_project WHERE id = :id;");
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

static bool bind_params(BXDatabaseQuery *query, BXObjectProject *project) {
  uint64_t now = 0;
  now = time(NULL);
  if (!bxd_bind(project, id) || !bxd_bind(project, uuid) ||
      !bxd_bind(project, nr) || !bxd_bind(project, name) ||
      !bxd_bind(project, start_date) || !bxd_bind(project, end_date) ||
      !bxd_bind(project, comment) || !bxd_bind(project, pr_state_id) ||
      !bxd_bind(project, pr_project_type_id) ||
      !bxd_bind(project, contact_id) || !bxd_bind(project, contact_sub_id) ||
      !bxd_bind(project, pr_invoice_type_id) ||
      !bxd_bind(project, pr_invoice_type_amount) ||
      !bxd_bind(project, pr_budget_type_id) ||
      !bxd_bind(project, pr_budget_type_amount) ||
      !bx_database_add_param_uint64(query, ":_checksum", &project->checksum) ||
      !bx_database_add_param_uint64(query, ":_last_updated", &now)) {
    return false;
  }
  return true;
}

BXillError execute_request(MYSQL *conn, BXObjectProject *project,
                           const char *request) {
  BXDatabaseQuery *query = bx_database_new_query(conn, request);
  if (query == NULL) {
    return ErrorGeneric;
  }
  BXillError retval = NoError;
  if (!bind_params(query, project)) {
    retval = ErrorGeneric;
    goto exit_point;
  }
  if (!bx_database_execute(query)) {
    if (query->need_reconnect) {
      retval = ErrorSQLReconnect;
    } else {
      retval = ErrorGeneric;
    }
    goto exit_point;
  }
  if (!bx_database_results(query)) {
    retval = ErrorGeneric;
  }

exit_point:
  bx_database_free_query(query);
  return retval;
}

BXillError bx_project_update_db(MYSQL *conn, BXObjectProject *project) {
  return execute_request(conn, project, QUERY_UPDATE);
}

BXillError bx_project_insert_db(MYSQL *conn, BXObjectProject *project) {
  return execute_request(conn, project, QUERY_INSERT);
}

BXillError _bx_project_sync_item(MYSQL *conn, json_t *item, Cache *cache) {
  BXillError RetVal = NoError;
  BXObjectProject *project = decode_object(item);
  if (project == NULL) {
    return ErrorGeneric;
  }
  CacheState ProjectState =
      cache_check_item(cache, (BXGeneric *)&project->id, project->checksum);
  if (ProjectState == CacheOk) {
    bx_project_free(project);
    return NoError;
  }

  if (ProjectState == CacheNotSet) {
    BXillError e = bx_project_insert_db(conn, project);
    if (e != NoError) {
      bx_log_error("Failed insert project %ld", project->id.value);
      RetVal = e;
      goto fail_and_return;
    }
  } else if (ProjectState == CacheNotSync) {
    BXillError e = bx_project_update_db(conn, project);
    if (e != NoError) {
      bx_log_error("Failed insert language %d", project->id.value);
      RetVal = e;
      goto fail_and_return;
    }
  }
  cache_set_item(cache, (BXGeneric *)&project->id, project->checksum);
  bx_project_free(project);
  return NoError;

fail_and_return:
  bx_project_free(project);
  return RetVal;
}

BXillError bx_project_sync_item(bXill *app, MYSQL *conn, BXGeneric *item,
                                Cache *cache) {
  bx_log_debug("Sync Project Id %ld", ((BXUInteger *)item)->value);
  BXNetRequest *request =
      bx_do_request(app->queue, NULL, GET_PROJECT_PATH, item);
  if (request == NULL) {
    return false;
  }

  if (request->response == NULL || request->response->http_code != 200) {
    return false;
  }

  bx_net_request_free(request);
  return _bx_project_sync_item(conn, request->decoded, cache);
}

#define WALK_PROJECT_PATH "2.0/pr_project?limit=$&offset=$"
BXillError bx_project_walk_item(bXill *app, MYSQL *conn, Cache *cache) {
  bx_log_debug("BX Walk Project Items");
  int len0hit_count = 0;
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BX_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    arr_len = 0;
    BXNetRequest *request =
        bx_do_request(app->queue, NULL, WALK_PROJECT_PATH, &limit, &offset);
    if (request == NULL) {
      return ErrorNet;
    }
    if (!json_is_array(request->decoded)) {
      bx_net_request_free(request);
      return ErrorJSON;
    }

    arr_len = json_array_size(request->decoded);
    if (arr_len == 0) {
      len0hit_count++;
    } else {
      len0hit_count = 0;
    }
    for (size_t i = 0; i < arr_len; i++) {
      BXillError e = _bx_project_sync_item(
          conn, json_array_get(request->decoded, i), cache);
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
