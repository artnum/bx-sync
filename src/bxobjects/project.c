#include "../include/bxobjects/project.h"
#include "../include/bx_database.h"
#include "../include/bx_net.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"
#include "../include/bxobjects/contact.h"
#include <jansson.h>
#include <stdint.h>
#include <threads.h>

#define GET_PROJECT_PATH "2.0/pr_project/$"
#define WALK_PROJECT_PATH "2.0/pr_project"
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
  "INSERT INTO pr_project (id, uuid, nr, name, start_date,"                    \
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

bool execute_request(MYSQL *conn, BXObjectProject *project,
                     const char *request) {
  BXDatabaseQuery *query = bx_database_new_query(conn, request);
  if (query == NULL) {
    return false;
  }
  bool success = true;
  if (!bind_params(query, project) || !bx_database_execute(query) ||
      !bx_database_results(query)) {
    success = false;
  }
  bx_database_free_query(query);
  return success;
}

bool bx_project_update_db(MYSQL *conn, BXObjectProject *project) {
  return execute_request(conn, project, QUERY_UPDATE);
}

bool bx_project_insert_db(MYSQL *conn, BXObjectProject *project) {
  return execute_request(conn, project, QUERY_INSERT);
}

bool bx_project_sync_item(bXill *app, MYSQL *conn, BXGeneric *item) {
  bx_log_debug("Sync Project Id %ld", ((BXUInteger *)item)->value);
  BXNetRequest *request =
      bx_do_request(app->queue, NULL, GET_PROJECT_PATH, item);
  if (request == NULL) {
    return false;
  }

  if (request->response == NULL || request->response->http_code != 200) {
    return false;
  }

  bx_log_debug("PROJECT %s", request->response->data);

  BXObjectProject *project = decode_object(request->decoded);
  bx_net_request_free(request);
  if (project == NULL) {
    return false;
  }
  if (!bx_contact_sync_item(app, conn, (BXGeneric *)&project->contact_id)) {
    bx_project_free(project);
    return false;
  }

  switch (bx_project_check_database(conn, project)) {
  default:
  case NeedNothing:
    break;
  case Error:
    bx_log_error("Error checking for project %ld", project->id.value);
    break;
  case NeedCreate:
    if (!bx_project_insert_db(conn, project)) {
      bx_log_error("Failed insert project %ld", project->id.value);
    }
    break;
  case NeedUpdate:
    if (!bx_project_insert_db(conn, project)) {
      bx_log_error("Failed insert language %d", project->id.value);
    }
    break;
  }

  bx_project_free(project);
  return true;
}

void bx_project_walk_item(bXill *app, MYSQL *conn) {
  bx_log_debug("BX Walk Project Items");
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BX_LIST_LIMIT};
  size_t arr_len = 0;
  do {
    arr_len = 0;
    BXNetRequest *request = bx_do_request(app->queue, NULL, WALK_PROJECT_PATH);
    if (request == NULL) {
      return;
    }
    if (!json_is_array(request->decoded)) {
      bx_net_request_free(request);
      return;
    }

    arr_len = json_array_size(request->decoded);
    for (size_t i = 0; i < arr_len; i++) {
      BXInteger id = bx_object_get_json_int(json_array_get(request->decoded, i),
                                            "id", NULL);
      bx_project_sync_item(app, conn, (BXGeneric *)&id);
    }
    bx_net_request_free(request);
    thrd_yield();
    offset.value += limit.value;
  } while (arr_len > 0);
}
