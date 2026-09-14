#include "include/bx_prune.h"
#include "include/bx_conf.h"
#include "include/bx_database.h"
#include "include/bx_ids_cache.h"
#include "include/bx_utils.h"
#include "include/bxill.h"
#include <mysql/mysql.h>

BXillError bx_prune_items(bXill *app, PruningParameters *param) {
  CacheIter iter;

  cache_iter_init(param->cache, &iter);
  int drift = bx_conf_get_int(app->conf, "prune-drift");
  if (drift == 0) {
    drift = BXILL_DEFAULT_DRIFT;
  }

  if (param->query == NULL) {
    return ErrorGeneric;
  }
  uint64_t id;
  while (cache_iter_next_prunable_id(&iter, drift, &id)) {
    bx_log_debug("Prunning %lu\n", (unsigned long)id);
    if (!bx_database_add_param_uint64(param->query, ":id", &id) ||
        !bx_database_execute(param->query))
    {
      bx_log_debug("Query failed %s", param->query);
      BXillError e =
          param->query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
      bx_database_free_result(param->query);
      return e;
    }
    cache_tombstone(param->cache, id);
  }
  bx_database_free_result(param->query);
  cache_prune(param->cache);
  return NoError;
}

BXillError bx_prune_from_db(bXill *app, PruningParameters *param) {
  if (param->query == NULL) {
    return ErrorGeneric;
  }
  if (bx_database_execute(param->query) && bx_database_results(param->query)) {
    for (BXDatabaseRow *current = param->query->results; current;
         current = current->next) {
      if (current->column_count != 2) {
        continue;
      }
      cache_set_item(param->cache, (uint64_t)current->columns[0].i_value,
                     (uint64_t)current->columns[1].i_value);
    }
  } else {
    BXillError e =
        param->query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_result(param->query);
    return e;
  }
  bx_database_free_result(param->query);
  return NoError;
}
