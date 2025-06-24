#ifndef BX_IDS_CACHE
#define BX_IDS_CACHE

#include "bx_object_value.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  BXAny id;
  uint64_t checksum;
  uint64_t last_seen;
} CacheItem;

typedef struct {
  uint32_t size;
  uint32_t count;
  uint64_t version;
  CacheItem *items;
} Cache;

typedef struct {
  Cache *c;
  uint32_t current;
  uint64_t version;
} CacheIter;

typedef enum { CacheOk = 0, CacheNotSet, CacheNotSync } CacheState;

Cache *cache_create();
void cache_print(Cache *c);
bool cache_set_item(Cache *c, BXGeneric *item_id, uint64_t checksum);
void cache_destroy(Cache *c);
CacheState cache_check_item(Cache *c, BXGeneric *item_id, uint64_t checksum);
void cache_store(Cache *c, const char *filename);
void cache_load(Cache *c, const char *filename);
void cache_stats(Cache *c, const char *name);

/**
 * Init a cache iterator. The cache iterator is set at the cache version on init
 * so any operation involving versionning will be at a stable value.
 *
 * @param[in]  c    The cache pointer
 * @param[out] iter Iterator to init, it is not allocated.
 */
void cache_iter_init(Cache *c, CacheIter *iter);
/**
 * Get next id in cache.
 *
 * @param[in] A cache iterator
 *
 * @return A pointer to the ID
 */
const BXGeneric *cache_iter_next_id(CacheIter *iter);

/**
 * Get the next ID that is prunable (last_seen drifted away from version too
 * much)
 *
 * @param[in] iter  A cache iterator
 * @param[in] drift The minimal dift to be considered prunable.
 * @param[in] del   Delete the item from the cache. Deletion just set "last
 * seen" to 0, you need to call cache_prune to remove from the cache.
 *
 * @return A pointer to the ID
 */
const BXGeneric *cache_iter_next_prunable_id(CacheIter *iter, uint64_t drift,
                                             bool del);
/**
 * Prune the cache phyisically removing deleted value
 *
 * @param[in] c The cache object to prunable
 */
void cache_prune(Cache *c);
void cache_empty(Cache *c);
#endif /* BX_IDS_CACHE */
