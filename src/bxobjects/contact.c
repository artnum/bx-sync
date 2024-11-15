#include "bx_object_value.h"
#include "bx_utils.h"
#include <bxobjects/contact.h>
#include <bx_object.h>
#include <jansson.h>
#include <stddef.h>

static const char * type = "contact";
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
    bx_object_free_value(&contact->remote_branch_ids);
    bx_object_free_value(&contact->remote_updated_at);
    bx_object_free_value(&contact->remote_profile_image);

    free(contact);
}

static void * get(BXObjectContact * contact, const char * key)
{
    if (contact == NULL) { return NULL; }
    if (key == NULL) { return NULL; }
    if (strcasecmp(key, "postcode") == 0) { return &contact->remote_postcode; }
    if (strcasecmp(key, "nr") == 0) { return &contact->remote_nr; }
    if (strcasecmp(key, "name_1") == 0) { return &contact->remote_name_1; }
    if (strcasecmp(key, "name_2") == 0) { return &contact->remote_name_2; }
    if (strcasecmp(key, "birthday") == 0) { return &contact->remote_birthday; }
    if (strcasecmp(key, "address") == 0) { return &contact->remote_address; }
    if (strcasecmp(key, "city") == 0) { return &contact->remote_city; }
    if (strcasecmp(key, "mail") == 0) { return &contact->remote_mail; }
    if (strcasecmp(key, "mail_second") == 0) { return &contact->remote_mail_second; }
    if (strcasecmp(key, "phone_fixed") == 0) { return &contact->remote_phone_fixed; }
    if (strcasecmp(key, "phone_fixed_second") == 0) { return &contact->remote_phone_fixed_second; }
    if (strcasecmp(key, "phone_mobile") == 0) { return &contact->remote_phone_mobile; }
    if (strcasecmp(key, "fax") == 0) { return &contact->remote_fax; }
    if (strcasecmp(key, "url") == 0) { return &contact->remote_url; }
    if (strcasecmp(key, "skype_name") == 0) { return &contact->remote_skype_name; }
    if (strcasecmp(key, "remarks") == 0) { return &contact->remote_remarks; }
    if (strcasecmp(key, "language_id") == 0) { return &contact->remote_language_id; }
    if (strcasecmp(key, "contact_groupd_ids") == 0) { return &contact->remote_contact_groupd_ids; }
    if (strcasecmp(key, "branch_ids") == 0) { return &contact->remote_branch_ids; }
    if (strcasecmp(key, "updated_at") == 0) { return &contact->remote_updated_at; }
    if (strcasecmp(key, "profile_image") == 0) { return &contact->remote_profile_image; }
    return NULL;
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
    contact->type = type;
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
    contact->remote_contact_groupd_ids =        bx_object_get_json_string(object, "contact_groupd_ids", hashState);
    contact->remote_branch_ids =                bx_object_get_json_string(object, "branch_ids", hashState);
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
    _bx_dump_any("contact_groupd_ids", &contact->remote_contact_groupd_ids, 1);
    _bx_dump_any("branch_ids", &contact->remote_branch_ids, 1);
    _bx_dump_any("profile_image", &contact->remote_profile_image, 1);
}