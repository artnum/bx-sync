#ifndef BX_OBJECT_USER_H__
#define BX_OBJECT_USER_H__

#include <bx_object_value.h>
#include <bxill.h>

typedef struct s_BXObjectUser BXObjectUser;
struct s_BXObjectUser {
    enum e_BXObjectType type;

    uint64_t checksum;

    BXUInteger remote_id;
    BXString remote_salutation_type;
    BXString remote_firstname;
    BXString remote_lastname;
    BXString remote_email;
    BXBool remote_is_superadmin;
    BXBool remote_is_accountant;
};

bool bx_user_sync_item(bXill * app, BXGeneric * item);

#endif /* BX_OBJECT_USER_H__ */