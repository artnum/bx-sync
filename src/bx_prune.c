#include "include/bx_prune.h"
#include "include/bx_conf.h"
#include "include/bx_database.h"
#include "include/bx_ids_cache.h"
#include "include/bx_object_value.h"
#include "include/bx_utils.h"
#include "include/bxill.h"
#include <mysql/mysql.h>

void bx_prune_items(bXill *app, PruningParameters *param) {
  CacheIter iter;

  cache_iter_init(param->cache, &iter);
  int drift = bx_conf_get_int(app->conf, "prune-drift");
  if (drift == 0) {
    drift = BXILL_DEFAULT_DRIFT;
  }

  if (param->query == NULL) {
    return;
  }
  const BXGeneric *id;
  while ((id = cache_iter_next_prunable_id(&iter, drift, true)) != NULL) {
    bx_log_debug("Prunning %lu\n", ((const BXUInteger *)id)->value);
    if (bx_database_add_bxtype(param->query, ":id", id)) {
      if (!bx_database_execute(param->query)) {
        bx_log_debug("Query failed %s", param->query);
      }
    }
  }
  bx_database_free_result(param->query);
  cache_prune(param->cache);
}

void bx_prune_from_db(bXill *app, PruningParameters *param) {
  if (param->query == NULL) {
    return;
  }
  cache_next_version(param->cache);

  if (bx_database_execute(param->query) && bx_database_results(param->query)) {
    for (BXDatabaseRow *current = param->query->results; current;
         current = current->next) {
      if (current->column_count != 2) {
        continue;
      }
      BXUInteger item = {.type = BX_OBJECT_TYPE_UINTEGER,
                         .value = (uint64_t)current->columns[0].i_value,
                         .isset = true};
      cache_set_item(param->cache, (BXGeneric *)&item,
                     (uint64_t)current->columns[1].i_value);
    }
  }
  bx_database_free_result(param->query);

  cache_invalidate(param->cache, 1);
  cache_prune(param->cache);
}
