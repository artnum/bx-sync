#ifndef BX_OBJECT_CONTACT_H__
#define BX_OBJECT_CONTACT_H__

#include <bx_object.h>
#include <bx_object_value.h>
#include <jansson.h>

typedef struct s_BXObjectContact BXObjectContact;
struct s_BXObjectContact {
    enum e_BXObjectType type;

    uint64_t id;
    uint64_t checksum;

    BXInteger remote_id;
    BXInteger remote_contact_type_id;
    BXInteger remote_salutation_id;
    BXInteger remote_country_id;
    BXInteger remote_user_id;
    BXInteger remote_owner_id;
    BXInteger remote_title_id;
    BXInteger remote_salutation_form;

    BXString remote_postcode;
    BXString remote_nr;
    BXString remote_name_1;
    BXString remote_name_2;
    BXString remote_birthday;
    BXString remote_address;
    BXString remote_city;
    BXString remote_mail;
    BXString remote_mail_second;
    BXString remote_phone_fixed;
    BXString remote_phone_fixed_second;
    BXString remote_phone_mobile;
    BXString remote_fax;
    BXString remote_url;
    BXString remote_skype_name;
    BXString remote_remarks;
    BXString remote_language_id;
    BXString remote_contact_groupd_ids;
    BXString remote_branch_ids;
    BXString remote_updated_at;
    BXString remote_profile_image;

    BXBool remote_is_lead;
};

void bx_object_contact_dump(void * data);
void * bx_object_contact_decode(void * object);
void bx_object_contact_free(void * data);

#endif /* BX_OBJECT_CONTACT_H__ */