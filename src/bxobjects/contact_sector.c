#include <bxobjects/contact_sector.h>
#include <bx_database.h>
#include <bx_object.h>
#include <bxill.h>
#include <bx_object_value.h>
#include <bx_utils.h>
#include <jansson.h>


static inline BXObjectContactSector * decode_object(json_t * root) 
{
    json_t * object = (json_t *)root;
    BXObjectContactSector * contact_sector = NULL;
    XXH3_state_t * hashState = XXH3_createState();
    if (hashState == NULL) {
        return NULL;
    }
    XXH3_64bits_reset(hashState);
    contact_sector = calloc(1, sizeof(*contact_sector));
    if (contact_sector == NULL) {
        return NULL;
    }
    contact_sector->type = BXTypeContactSector;
    contact_sector->remote_id =   bx_object_get_json_int(object, "id", hashState);
    contact_sector->remote_name = bx_object_get_json_string(object, "name", hashState);
    
    contact_sector->checksum = XXH3_64bits_digest(hashState);
    XXH3_freeState(hashState);

    return contact_sector;    
}

#define WALK_CONTACT_SECTOR_PATH    "2.0/contact_branch"
bool bx_contact_sector_walk_items(bXill * app)
{
    BXNetRequest * request = bx_do_request(app->queue, NULL, WALK_CONTACT_SECTOR_PATH);
    if(request == NULL) {
        return false;
    }
    json_t * contact_sector_array = request->decoded;
    /* bx_net_request_free decref json_t, so we incref here to keep it for the run */
    json_incref(contact_sector_array);
    bx_net_request_free(request);
    size_t contact_sector_array_count = json_array_size(contact_sector_array);
    for (size_t i = 0; i < contact_sector_array_count; i++) {
        BXObjectContactSector * contact_sector = decode_object(json_array_get(contact_sector_array, i));
        
        if (contact_sector == NULL) {
            continue;
        }

        time_t now = time(NULL);
        BXDatabaseQuery * query = bx_database_new_query(
            app->mysql,
            "SELECT _checksum FROM contact_sector WHERE id = :id;"
        );
        bx_database_add_param_int64(query, ":id", &contact_sector->remote_id.value);
        bx_database_execute(query);
        bx_database_results(query);
        if (query->results == NULL || query->results->column_count == 0) {
            bx_database_free_query(query);
            query = bx_database_new_query(
                app->mysql,
                "INSERT INTO contact_sector (_checksum, id, name, _last_updated)"
                "VALUES (:_checksum, :id, :name, :_last_updated);"
            );
            bx_database_add_param_char(query, ":name", contact_sector->remote_name.value, contact_sector->remote_name.value_len);
            bx_database_add_param_uint64(query, ":_checksum", &contact_sector->checksum);
            bx_database_add_param_int64(query, ":id", &contact_sector->remote_id.value);
            bx_database_add_param_uint64(query, ":_last_updated", &now);
            bx_database_execute(query);
            bx_database_free_query(query);
            continue;
        }
        if (query->results[0].columns[0].i_value == contact_sector->checksum) {
            bx_database_free_query(query);
            continue;
        }

        bx_database_free_query(query);
        query = bx_database_new_query(
            app->mysql,
            "UPDATE contact_sector SET _checksum = :_checksum, name = :name, "
            "_last_updated = :_last_updated, _deleted = :_deleted;"
        );
        uint64_t not_deleted = 0;
        bx_database_add_param_char(query, ":name", contact_sector->remote_name.value, contact_sector->remote_name.value_len);
        bx_database_add_param_uint64(query, ":_checksum", &contact_sector->checksum);
        bx_database_add_param_uint64(query, ":_last_updated", &now);
        bx_database_add_param_uint64(query, ":_deleted", &not_deleted);
        bx_database_execute(query);
        bx_database_free_query(query);
    }
    json_decref(contact_sector_array);
    return true;
}