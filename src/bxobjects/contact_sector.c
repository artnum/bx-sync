#include "../include/bxobjects/contact_sector.h"
#include "../include/bx_database.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"
#include <jansson.h>

static inline void free_object(BXObjectContactSector *contact_sector) {
  if (contact_sector == NULL) {
    return;
  }
  bx_object_free_value(&contact_sector->remote_id);
  bx_object_free_value(&contact_sector->remote_name);
  free(contact_sector);
}

static inline BXObjectContactSector *decode_object(json_t *root) {
  json_t *object = (json_t *)root;
  BXObjectContactSector *contact_sector = NULL;
  XXH3_state_t *hashState = XXH3_createState();
  if (hashState == NULL) {
    return NULL;
  }
  XXH3_64bits_reset(hashState);
  contact_sector = calloc(1, sizeof(*contact_sector));
  if (contact_sector == NULL) {
    return NULL;
  }
  contact_sector->type = BXTypeContactSector;
  contact_sector->remote_id = bx_object_get_json_uint(object, "id", hashState);
  contact_sector->remote_name =
      bx_object_get_json_string(object, "name", hashState);

  contact_sector->checksum = XXH3_64bits_digest(hashState);
  XXH3_freeState(hashState);

  return contact_sector;
}

struct s_ItemMustBeDeleted {
  int64_t item;
  bool deleted;
};

#define WALK_CONTACT_SECTOR_PATH "2.0/contact_branch"
bool bx_contact_sector_walk_items(bXill *app) {
  BXNetRequest *request =
      bx_do_request(app->queue, NULL, WALK_CONTACT_SECTOR_PATH);
  if (request == NULL || request->response == NULL ||
      request->response->http_code != 200) {
    return false;
  }

  struct s_ItemMustBeDeleted *items = NULL;
  int items_count = 0;
  json_t *contact_sector_array = request->decoded;
  /* bx_net_request_free decref json_t, so we incref here to keep it for the run
   */
  json_incref(contact_sector_array);
  bx_net_request_free(request);

  size_t contact_sector_array_count = json_array_size(contact_sector_array);

  /* load all actual id from database, set them to deleted. If they are found
   * on the remote side, we set deleted to false and deletes only those still
   * set to true.
   * we don't really care if we fail to get those values as we prefer to have
   * too much data than not enough
   */
  BXDatabaseQuery *query_ids = bx_database_new_query(
      app->mysql, "SELECT id FROM contact_sector WHERE _deleted = 0;");
  if (query_ids != NULL) {
    if (bx_database_execute(query_ids)) {
      bx_database_results(query_ids);
      if (query_ids->results != NULL && query_ids->row_count > 0) {
        items = calloc(query_ids->row_count, sizeof(*items));
        if (items != NULL) {
          int i = 0;
          for (BXDatabaseRow *current = query_ids->results; current != NULL;
               current = current->next) {
            items[i].deleted = true;
            items[i].item = current->columns[0].i_value;
            i++;
          }
          items_count = query_ids->row_count;
        }
      }
    }
  }
  bx_database_free_query(query_ids);

  for (size_t i = 0; i < contact_sector_array_count; i++) {
    json_t *o = json_array_get(contact_sector_array, i);
    BXObjectContactSector *contact_sector = decode_object(o);

    /* set to "not delete" items already in database */
    if (items != NULL && items_count > 0) {
      for (int j = 0; j < items_count; j++) {
        if (items[j].item == contact_sector->remote_id.value) {
          items[j].deleted = false;
          break;
        }
      }
    }

    if (contact_sector == NULL) {
      continue;
    }

    time_t now = time(NULL);
    BXDatabaseQuery *query = bx_database_new_query(
        app->mysql, "SELECT _checksum FROM contact_sector WHERE id = :id;");
    bx_database_add_param_int64(query, ":id", &contact_sector->remote_id.value);
    bx_database_execute(query);
    bx_database_results(query);
    if (query->results == NULL || query->results->column_count == 0) {
      bx_database_free_query(query);
      query = bx_database_new_query(
          app->mysql,
          "INSERT INTO contact_sector (_checksum, id, name, _last_updated)"
          "VALUES (:_checksum, :id, :name, :_last_updated);");
      bx_database_add_param_char(query, ":name",
                                 contact_sector->remote_name.value,
                                 contact_sector->remote_name.value_len);
      bx_database_add_param_uint64(query, ":_checksum",
                                   &contact_sector->checksum);
      bx_database_add_param_uint64(query, ":id",
                                   &contact_sector->remote_id.value);
      bx_database_add_param_uint64(query, ":_last_updated", &now);
      bx_database_execute(query);
      bx_database_free_query(query);
      free_object(contact_sector);
      continue;
    }
    if (query->results[0].columns[0].i_value == contact_sector->checksum) {
      bx_database_free_query(query);
      free_object(contact_sector);
      continue;
    }

    bx_database_free_query(query);
    query = bx_database_new_query(
        app->mysql,
        "UPDATE contact_sector SET _checksum = :_checksum, name = :name, "
        "_last_updated = :_last_updated, _deleted = :_deleted;");
    uint64_t not_deleted = 0;
    bx_database_add_param_char(query, ":name",
                               contact_sector->remote_name.value,
                               contact_sector->remote_name.value_len);
    bx_database_add_param_uint64(query, ":_checksum",
                                 &contact_sector->checksum);
    bx_database_add_param_uint64(query, ":_last_updated", &now);
    bx_database_add_param_uint64(query, ":_deleted", &not_deleted);
    bx_database_execute(query);
    bx_database_free_query(query);
    free_object(contact_sector);
  }
  json_decref(contact_sector_array);

  if (items != NULL && items_count > 0) {
    int64_t _to_delete = 0;
    int64_t *to_delete = &_to_delete;
    time_t now = time(NULL);
    BXDatabaseQuery *query = bx_database_new_query(
        app->mysql,
        "UPDATE contact_sector SET _deleted = :_deleted  WHERE id = :id;");
    bx_database_add_param_int64(query, ":_deleted ", &now);
    bx_database_add_param_int64(query, ":id", to_delete);
    for (int i = 0; i < items_count; i++) {
      if (items[i].deleted) {
        *to_delete = items[i].item;
        bx_database_execute(query);
      }
    }
    bx_database_free_query(query);
  }
  if (items != NULL) {
    free(items);
  }

  return true;
}
