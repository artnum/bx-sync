#ifndef BX_PRUNE_H__
#define BX_PRUNE_H__
#include "bx_database.h"
#include "bx_ids_cache.h"
#include "bx_object_value.h"
#include "bxill.h"
#include <mysql/mysql.h>

typedef struct {
  Cache *cache;
  BXDatabaseQuery *query;
} PruningParameters;

void bx_prune_items(bXill *app, PruningParameters *param);
void bx_prune_from_db(bXill *app, PruningParameters *param);

#endif /* BX_PRUNE_H__ */
