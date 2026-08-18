#ifndef BX_OBJECT_ACCOUNT_H__
#define BX_OBJECT_ACCOUNT_H__

#include "../bx_object_value.h"
#include "../bxill.h"

typedef struct s_BXObjectAccount BXObjectAccount;
struct s_BXObjectAccount {
  enum e_BXObjectType type;
  uint64_t checksum;
  BXUInteger id;
  BXUuid uuid;
  BXString account_no;
  BXString name;
  BXInteger account_type;
  BXUInteger tax_id;
  BXUInteger fibu_account_group_id;
  BXBool is_active;
  BXBool is_locked;
};

BXillError bx_account_walk_items(bXill *app, MYSQL *conn);

#endif /* BX_OBJECT_ACCOUNT_H__ */
