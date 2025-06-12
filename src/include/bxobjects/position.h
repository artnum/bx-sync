#ifndef BX_OBJECT_POSITION_H__
#define BX_OBJECT_POSITION_H__

#include "../bx_object.h"
#include "../bx_object_value.h"
#include <stdint.h>

typedef struct s_BXObjectGenericPosition BXObjectGenericPosition;
struct s_BXObjectGenericPosition {
  enum e_BXObjectType type;
};

typedef struct s_BXObjectPositionCE BXObjectPositionCE;
struct s_BXObjectPositionCE {
  enum e_BXObjectType type;

  uint64_t id;
  uint64_t checksum;

  BXUInteger remote_id;
  BXUInteger remote_unit_id;
  BXUInteger remote_account_id;
  BXUInteger remote_tax_id;
  BXUInteger remote_internal_pos;
  BXUInteger remote_parent_id;

  BXString remote_unit_name;
  BXString remote_text;
  BXString remote_pos;

  BXBool remote_is_optional;

  BXFloat remote_amount;
  BXFloat remote_tax_value;
  BXFloat remote_unit_price;
  BXFloat remote_discount_in_percent;
  BXFloat remote_position_total;
};

typedef struct s_BXObjectPositionAE BXObjectPositionAE;
struct s_BXObjectPositionAE {
  enum e_BXObjectType type;

  uint64_t id;
  uint64_t checksum;

  BXUInteger remote_id;
  BXUInteger remote_unit_id;
  BXUInteger remote_account_id;
  BXUInteger remote_tax_id;
  BXUInteger remote_internal_pos;
  BXUInteger remote_parent_id;
  BXUInteger remote_article_id;

  BXString remote_unit_name;
  BXString remote_text;
  BXString remote_pos;

  BXBool remote_is_optional;

  BXFloat remote_amount;
  BXFloat remote_tax_value;
  BXFloat remote_unit_price;
  BXFloat remote_discount_in_percent;
  BXFloat remote_position_total;
};

typedef struct s_BXObjectPositionTE BXObjectPositionTE;
struct s_BXObjectPositionTE {
  enum e_BXObjectType type;

  uint64_t id;
  uint64_t checksum;

  BXUInteger remote_id;
  BXUInteger remote_internal_pos;
  BXUInteger remote_parent_id;

  BXString remote_text;
  BXString remote_pos;

  BXBool remote_is_optional;
  BXBool remote_show_pos_nr;
};

typedef struct s_BXObjectPositionSE BXObjectPositionSE;
struct s_BXObjectPositionSE {
  enum e_BXObjectType type;

  uint64_t id;
  uint64_t checksum;

  BXUInteger remote_id;
  BXUInteger remote_internal_pos;
  BXUInteger remote_parent_id;

  BXString remote_text;

  BXBool remote_is_optional;

  BXFloat remote_value;
};

typedef struct s_BXObjectPositionPE BXObjectPositionPE;
struct s_BXObjectPositionPE {
  enum e_BXObjectType type;

  uint64_t id;
  uint64_t checksum;

  BXUInteger remote_id;
  BXUInteger remote_internal_pos;
  BXUInteger remote_parent_id;

  BXBool remote_is_optional;
};

typedef struct s_BXObjectPositionDE BXObjectPositionDE;
struct s_BXObjectPositionDE {
  enum e_BXObjectType type;

  uint64_t id;
  uint64_t checksum;

  BXUInteger remote_id;

  BXString remote_text;

  BXBool remote_is_percentual;

  BXFloat remote_value;
  BXFloat remote_discount_total;
};

void bx_object_position_dump(void *data);
void bx_object_position_free(void *data);
void *bx_object_position_decode(void *object);

#endif /* BX_OBJECT_POSITION_H__ */
