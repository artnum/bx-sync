#include <bxobjects/contact_group.h>
#include <bx_database.h>
#include <bx_object.h>
#include <bxill.h>
#include <bx_object_value.h>
#include <bx_utils.h>


static inline BXObjectContactGroup * decode_object(json_t * root) 
{
    json_t * object = (json_t *)root;
    BXObjectContactGroup * contact_group = NULL;
    XXH3_state_t * hashState = XXH3_createState();
    if (hashState == NULL) {
        return NULL;
    }
    XXH3_64bits_reset(hashState);
    contact_group = calloc(1, sizeof(*contact_group));
    if (contact_group == NULL) {
        return NULL;
    }
    contact_group->type = BXTypeContactGroup;
    contact_group->remote_id = bx_object_get_json_int(object, "id", hashState);
    contact_group->remote_name = bx_object_get_json_string(object, "name", hashState);
    
    contact_group->checksum = XXH3_64bits_digest(hashState);
    XXH3_freeState(hashState);

    return contact_group;    
}

#define GET_CONTACT_GROUP_PATH    "2.0/contact_group/$"

bool bx_contact_group_sync_item(bXill * app, BXGeneric * item)
{
    BXNetRequest * request = bx_do_request(app->queue, NULL, GET_CONTACT_GROUP_PATH, item);
    if (request == NULL
        || request->response == NULL
        || request->response->http_code != 200
    ) {
        return false;
    }

    BXObjectContactGroup * contact_group = decode_object(request->decoded);
    bx_net_request_free(request);
    if (contact_group == NULL) {
        return false;
    }

    time_t now = time(NULL);
    BXDatabaseQuery * query = bx_database_new_query(
        app->mysql,
        "SELECT _checksum FROM contact_group WHERE id = :id;"
    );
    bx_database_add_param_int64(query, ":id", &contact_group->remote_id.value);
    bx_database_execute(query);
    bx_database_results(query);
    if (query->results == NULL || query->results->column_count == 0) {
        bx_database_free_query(query);
        query = bx_database_new_query(
            app->mysql,
            "INSERT INTO contact_group (_checksum, id, name, _last_updated)"
            "VALUES (:_checksum, :id, :name, :_last_updated);"
        );
        bx_database_add_param_char(query, ":name", contact_group->remote_name.value, contact_group->remote_name.value_len);
        bx_database_add_param_uint64(query, ":_checksum", &contact_group->checksum);
        bx_database_add_param_int64(query, ":id", &contact_group->remote_id.value);
        bx_database_add_param_uint64(query, ":_last_updated", &now);
        bx_database_execute(query);
        bx_database_free_query(query);
        return true;
    }
    if (query->results[0].columns[0].i_value == contact_group->checksum) {
        bx_database_free_query(query);
        return true;
    }

    bx_database_free_query(query);
    query = bx_database_new_query(
        app->mysql,
        "UPDATE contact_group SET _checksum = :_checksum, name = :name, "
        "_last_updated = :_last_updated, _deleted = :_deleted;"
    );
    uint64_t not_deleted = 0;
    bx_database_add_param_char(query, ":name", contact_group->remote_name.value, contact_group->remote_name.value_len);
    bx_database_add_param_uint64(query, ":_checksum", &contact_group->checksum);
    bx_database_add_param_uint64(query, ":_last_updated", &now);
    bx_database_add_param_uint64(query, ":_deleted", &not_deleted);
    bx_database_execute(query);
    bx_database_free_query(query);
    return true;
}