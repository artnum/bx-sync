#include "bx_decode.h"
#include "bx_net.h"
#include <bx_object_value.h>
#include <bx_utils.h>
#include <bxobjects/contact.h>
#include <bxobjects/user.h>
#include <bx_object.h>
#include <jansson.h>
#include <stddef.h>
#include <mysql/mysql.h>
#include <bx_database.h>
#include <bxill.h>
#include <unistd.h>

void bx_object_contact_free(void * data)
{
    BXObjectContact * contact = (BXObjectContact *)data;
    if (contact == NULL) { return; }
    
    bx_object_free_value(&contact->remote_postcode);
    bx_object_free_value(&contact->remote_nr);
    bx_object_free_value(&contact->remote_name_1);
    bx_object_free_value(&contact->remote_name_2);
    bx_object_free_value(&contact->remote_birthday);
    bx_object_free_value(&contact->remote_address);
    bx_object_free_value(&contact->remote_city);
    bx_object_free_value(&contact->remote_mail);
    bx_object_free_value(&contact->remote_mail_second);
    bx_object_free_value(&contact->remote_phone_fixed);
    bx_object_free_value(&contact->remote_phone_fixed_second);
    bx_object_free_value(&contact->remote_phone_mobile);
    bx_object_free_value(&contact->remote_fax);
    bx_object_free_value(&contact->remote_url);
    bx_object_free_value(&contact->remote_skype_name);
    bx_object_free_value(&contact->remote_remarks);
    bx_object_free_value(&contact->remote_language_id);
    bx_object_free_value(&contact->remote_contact_groupd_ids);
    bx_object_free_value(&contact->remote_contact_branch_ids);
    bx_object_free_value(&contact->remote_updated_at);
    bx_object_free_value(&contact->remote_profile_image);

    free(contact);
}

void bx_object_contact_store(MYSQL * mysql, BXObjectContact * contact)
{
    BXDatabaseQuery * query = bx_database_new_query(
        mysql, 
        "INSERT INTO contact (id, user_id, contact_type_id, country_id, "
        "owner_id, title_id, salutation_form, postcode, nr, name_1, name_2, "
        "address, birthday, updated_at, city, mail, mail_second, phone_fixed, "
        "phone_fixed_second, phone_mobile, phone_fax, url, skype_name, remarks, "
        "lanuage_id, contact_group_ids, branch_ids, profile_image, is_lead, "
        "_checksum, _id"
        "VALUES(:id, :user_id, :contact_type_id, :country_id, "
        ":owner_id, :title_id, :salutation_form, :postcode, :nr, :name_1, :name_2, "
        ":address, :birthday, :updated_at, :city, :mail, :mail_second, :phone_fixed, "
        ":phone_fixed_second, :phone_mobile, :phone_fax, url, :skype_name, :remarks, "
        ":lanuage_id, :contact_group_ids, :branch_ids, :profile_image, :is_lead, "
        ":_checksum, :_id)"
    );

    bx_database_add_param_int32(query, ":id", &contact->remote_id);
    bx_database_add_param_int32(query, ":user_id", &contact->remote_user_id);
    bx_database_add_param_int32(query, ":contact_type_id", &contact->remote_contact_type_id);
    bx_database_add_param_int32(query, ":salutation_id", &contact->remote_salutation_id);
    bx_database_add_param_int32(query, ":owner_id", &contact->remote_owner_id);
    bx_database_add_param_int32(query, ":title_id", &contact->remote_title_id);
    bx_database_add_param_int32(query, ":salutation_form", &contact->remote_salutation_form);

    int64_t * contact_group_ids = bx_int_string_array_to_int_array(contact->remote_contact_groupd_ids.value);
    if (contact_group_ids != NULL) {
  


        for (int i = 1; i <= contact_group_ids[0]; i++) {
            BXDatabaseQuery * select_cgi = bx_database_new_query(
                mysql,
                "SELECT group_id,contact_id FROM contact_group_to_contact_id "
                "WHERE group_id = :gid AND contact_id = :cid;"
            );
            bx_database_add_param_uint32(select_cgi, ":gid", &contact_group_ids[i]);
            bx_database_add_param_uint32(select_cgi, ":cid", &contact->remote_id.value);
            bx_database_execute(select_cgi);
            bx_database_results(select_cgi);

            if (select_cgi->results == NULL || select_cgi->results->column_count <= 0) {
                BXDatabaseQuery * insert_cgi = bx_database_new_query(
                    mysql, 
                    "INSERT INTO contact_group_to_contact_id (group_id, contact_id) "
                    "VALUES(:gid, :cid);"
                );
                if (select_cgi == NULL) {
                    /* TODO handle failure */
                }
                bx_database_add_param_uint32(insert_cgi, ":gid", &contact_group_ids[i]);
                bx_database_add_param_uint32(insert_cgi, ":cid", &contact->remote_id.value);
                bx_database_execute(insert_cgi);
                bx_database_free_query(insert_cgi);
            }
            bx_database_free_query(select_cgi);
        }
        free(contact_group_ids);
    }

    bx_database_free_query(query);
}

void * bx_object_contact_decode(void * jroot)
{
    json_t * object = (json_t *)jroot;
    BXObjectContact * contact = NULL;
    XXH3_state_t * hashState = XXH3_createState();
    if (hashState == NULL) {
        return NULL;
    }
    XXH3_64bits_reset(hashState);
    contact = calloc(1, sizeof(*contact));
    if (contact == NULL) {
        return NULL;
    }
    contact->type = BXTypeContact;
    bx_utils_gen_id(&contact->id);

    /* integer */
    contact->remote_id =                        bx_object_get_json_int(object, "id", hashState);
    contact->remote_user_id =                   bx_object_get_json_int(object, "user_id", hashState);
    contact->remote_contact_type_id =           bx_object_get_json_int(object, "contact_type_id", hashState);
    contact->remote_salutation_id =             bx_object_get_json_int(object, "salutation_id", hashState);
    contact->remote_country_id =                bx_object_get_json_int(object, "country_id", hashState);
    contact->remote_owner_id =                  bx_object_get_json_int(object, "owner_id", hashState);
    contact->remote_title_id =                  bx_object_get_json_int(object, "title_id", hashState);
    contact->remote_salutation_form =           bx_object_get_json_int(object, "salutation_form", hashState);

    /* string */
    contact->remote_postcode =                  bx_object_get_json_string(object, "postcode", hashState);
    contact->remote_nr =                        bx_object_get_json_string(object, "nr", hashState);
    contact->remote_name_1 =                    bx_object_get_json_string(object, "name_1", hashState);
    contact->remote_name_2 =                    bx_object_get_json_string(object, "name_2", hashState);
    contact->remote_address =                   bx_object_get_json_string(object, "address", hashState);
    contact->remote_birthday =                  bx_object_get_json_string(object, "birthday", hashState);
    contact->remote_updated_at =                bx_object_get_json_string(object, "updated_at", hashState);
    contact->remote_city =                      bx_object_get_json_string(object, "city", hashState);
    contact->remote_mail =                      bx_object_get_json_string(object, "mail", hashState);
    contact->remote_mail_second =               bx_object_get_json_string(object, "mail_second", hashState);
    contact->remote_phone_fixed =               bx_object_get_json_string(object, "phone_fixed", hashState);
    contact->remote_phone_fixed_second =        bx_object_get_json_string(object, "phone_fixed_second", hashState);
    contact->remote_phone_mobile =              bx_object_get_json_string(object, "phone_mobile", hashState);
    contact->remote_fax =                       bx_object_get_json_string(object, "fax", hashState);
    contact->remote_url =                       bx_object_get_json_string(object, "url", hashState);
    contact->remote_skype_name =                bx_object_get_json_string(object, "skype_name", hashState);
    contact->remote_remarks =                   bx_object_get_json_string(object, "remarks", hashState);
    contact->remote_language_id =               bx_object_get_json_string(object, "language_id", hashState);
    contact->remote_contact_groupd_ids =        bx_object_get_json_string(object, "contact_group_ids", hashState);
    contact->remote_contact_branch_ids =                bx_object_get_json_string(object, "contact_branch_ids", hashState);
    contact->remote_profile_image =             bx_object_get_json_string(object, "profile_image", hashState);

    contact->remote_is_lead =                   bx_object_get_json_bool(object, "is_lead", hashState);

    contact->checksum = XXH3_64bits_digest(hashState);
    XXH3_freeState(hashState);
    return contact;
}

void bx_object_contact_dump(void * data)
{
    BXObjectContact * contact = (BXObjectContact *)data;
    if (contact == NULL) { return; }

    _bx_dump_print_title("### DUMP CONTACT '%s' ID:%lx CS:%lx ###", contact->remote_nr.value, contact->id, contact->checksum);
    _bx_dump_any("id", &contact->remote_id, 1);
    _bx_dump_any("user_id", &contact->remote_user_id, 1);
    _bx_dump_any("contact_type_id", &contact->remote_contact_type_id, 1);
    _bx_dump_any("salutation_id", &contact->remote_salutation_id, 1);
    _bx_dump_any("country_id", &contact->remote_country_id, 1);
    _bx_dump_any("owner_id", &contact->remote_owner_id, 1);
    _bx_dump_any("title_id", &contact->remote_title_id, 1);
    _bx_dump_any("salutation_form", &contact->remote_salutation_form, 1);
    _bx_dump_any("postcode", &contact->remote_postcode, 1);
    _bx_dump_any("nr", &contact->remote_nr, 1);
    _bx_dump_any("name_1", &contact->remote_name_1, 1);
    _bx_dump_any("name_2", &contact->remote_name_2, 1);
    _bx_dump_any("address", &contact->remote_address, 1);
    _bx_dump_any("birthday", &contact->remote_birthday, 1);
    _bx_dump_any("updated_at", &contact->remote_updated_at, 1);
    _bx_dump_any("city", &contact->remote_city, 1);
    _bx_dump_any("mail", &contact->remote_mail, 1);
    _bx_dump_any("mail_second", &contact->remote_mail_second, 1);
    _bx_dump_any("phone_fixed", &contact->remote_phone_fixed, 1);
    _bx_dump_any("phone_fixed_second", &contact->remote_phone_fixed_second, 1);
    _bx_dump_any("phone_mobile", &contact->remote_phone_mobile, 1);
    _bx_dump_any("fax", &contact->remote_fax, 1);
    _bx_dump_any("url", &contact->remote_url, 1);
    _bx_dump_any("skype_name", &contact->remote_skype_name, 1);
    _bx_dump_any("remarks", &contact->remote_remarks, 1);
    _bx_dump_any("language_id", &contact->remote_language_id, 1);
    _bx_dump_any("contact_group_ids", &contact->remote_contact_groupd_ids, 1);
    _bx_dump_any("contact_branch_ids", &contact->remote_contact_branch_ids, 1);
    _bx_dump_any("profile_image", &contact->remote_profile_image, 1);
}

#define GET_CONTACT_PATH    "2.0/contact/$"
bool bx_contact_sync_item(bXill * app, BXGeneric * item)
{
    assert(app != NULL);
    assert(item != NULL);
    printf("Contact %ld\n", ((BXInteger *)item)->value);
    BXNetRequest * request = bx_do_request(app->queue, NULL, GET_CONTACT_PATH, item);
    if(request == NULL) {
        return false;
    }
    if (request->response == NULL || request->response->http_code != 200) {
        return false;
    }
    BXObjectContact * contact = bx_object_contact_decode(request->decoded);
    bx_net_request_free(request);
    if (contact == NULL) {
        return false;
    }

    bx_user_sync_item(app, (BXGeneric *)&contact->remote_user_id);
    if (contact->remote_user_id.value != contact->remote_owner_id.value) {
        bx_user_sync_item(app, (BXGeneric *)&contact->remote_owner_id);
    }

    int64_t * group_ids = bx_int_string_array_to_int_array(contact->remote_contact_groupd_ids.value);
    if (group_ids != NULL) {
        BXInteger item;
        item.isset = true;
        item.type = BX_OBJECT_TYPE_INTEGER;
        for (int i = 0; i <= *group_ids; i++) {
            item.value = *(group_ids + i);
            if (!bx_contact_group_sync_item(app, (BXGeneric *)&item)) {
                continue;
            }
        }
        free(group_ids);
    }

    int64_t * branch_ids = bx_int_string_array_to_int_array(contact->remote_contact_branch_ids.value);
    if (branch_ids != NULL) {
        free(branch_ids);
    }
    bx_object_contact_free(contact);
    
    return true;
}

#define WALK_CONTACT_PATH    "2.0/contact"
void bx_contact_walk_items(bXill * app)
{
    BXNetRequest * request = bx_do_request(app->queue, NULL, WALK_CONTACT_PATH);
    if(request == NULL) {
        return;
    }
    if (!json_is_array(request->decoded)) {
        bx_net_request_free(request);
        return;
    }

    size_t arr_len = json_array_size(request->decoded);
    for (size_t i = 0; i < arr_len; i++) {
        BXInteger id = bx_object_get_json_int(json_array_get(request->decoded, i), "id", NULL);
        bx_contact_sync_item(app, (BXGeneric *)&id);
    }
    bx_net_request_free(request);
}