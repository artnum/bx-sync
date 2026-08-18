#ifndef BX_OBJECT_CONTACT_GROUP_H__
#define BX_OBJECT_CONTACT_GROUP_H__

#include "../bx_object_value.h"
#include "../bxill.h"

typedef struct s_BXObjectContactGroup BXObjectContactGroup;
struct s_BXObjectContactGroup {
  enum e_BXObjectType type;

  uint64_t checksum;

  BXUInteger remote_id;
  BXString remote_name;
};

bool bx_contact_group_sync_item(bXill *app, MYSQL *conn, BXGeneric *item);
BXillError bx_contact_group_walk_items(bXill *app, MYSQL *conn);
BXillError bx_contact_group_link(MYSQL *conn, uint64_t contact_id,
                                 const char *group_ids);

#endif /* BX_OBJECT_CONTACT_GROUP_H__ */
