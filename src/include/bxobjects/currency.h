#ifndef BX_OBJECT_CURRENCY_H__
#define BX_OBJECT_CURRENCY_H__

#include "../bx_object_value.h"
#include "../bxill.h"

typedef struct s_BXObjectCurrency BXObjectCurrency;
struct s_BXObjectCurrency {
  enum e_BXObjectType type;
  uint64_t checksum;
  BXUInteger id;
  BXString name;
  BXFloat round_factor;
  BXFloat exchange_rate;
  BXUInteger exchange_rate_id;
  BXFloat ratio;
  BXString source;
  BXString source_reason;
  BXString exchange_rate_date;
};

BXillError bx_currency_walk_items(bXill *app, MYSQL *conn);

#endif /* BX_OBJECT_CURRENCY_H__ */
