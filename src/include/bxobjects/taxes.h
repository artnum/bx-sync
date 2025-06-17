#ifndef BX_OBJECT_TAX_H__
#define BX_OBJECT_TAX_H__

#include "../bx_object.h"
#include "../bx_object_value.h"
#include <stdint.h>

typedef struct s_BXObjectTax BXObjectTax;
struct s_BXObjectTax {
  enum e_BXObjectType type;

  BXUInteger id;
  BXUuid uuid;
  BXString name;
  BXInteger digit;
  BXString taxtype;
  BXUInteger account_id;
  BXString tax_settlement_type;
  BXFloat value;
  BXString net_tax_value;
  BXInteger start_year;
  BXInteger end_year;
  BXBool is_active;
  BXString display_name;
  BXInteger start_month;
  BXInteger end_month;

  uint64_t checksum;
};

void *bx_object_tax_decode(void *jroot);
void bx_object_tax_free(void *data);

#endif /* BX_OBJECT_TAX_H__ */
