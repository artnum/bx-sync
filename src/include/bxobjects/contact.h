#ifndef BX_OBJECT_CONTACT_H__
#define BX_OBJECT_CONTACT_H__

#include "../bx_ids_cache.h"
#include "../bx_object.h"
#include "../bx_object_value.h"
#include "contact_group.h"
#include <jansson.h>

typedef struct s_BXObjectContact BXObjectContact;
struct s_BXObjectContact {
  enum e_BXObjectType type;

  uint64_t checksum;

  BXUInteger id;
  BXUInteger contact_type_id;
  BXUInteger salutation_id;
  BXUInteger country_id;
  BXUInteger user_id;
  BXUInteger owner_id;
  BXUInteger title_id;
  BXUInteger salutation_form;
  BXUInteger language_id;

  BXString postcode;
  BXString nr;
  BXString name_1;
  BXString name_2;
  BXString birthday;
  BXString address;
  BXString city;
  BXString mail;
  BXString mail_second;
  BXString phone_fixed;
  BXString phone_fixed_second;
  BXString phone_mobile;
  BXString fax;
  BXString url;
  BXString skype_name;
  BXString remarks;
  BXString contact_groupd_ids;
  BXString contact_branch_ids;
  BXString updated_at;
  BXString profile_image;
};

void bx_object_contact_dump(void *data);
void *bx_object_contact_decode(void *object);
void bx_object_contact_free(void *data);
void bx_object_contact_store(MYSQL *mysql, BXObjectContact *contact);
bool bx_contact_sync_item(bXill *app, MYSQL *conn, BXGeneric *item,
                          BXBool show_archived, Cache *c);
void bx_contact_walk_items(bXill *app, MYSQL *conn, Cache *c);
bool bx_contact_is_in_database(MYSQL *conn, BXGeneric *item);
#endif /* BX_OBJECT_CONTACT_H__ */
