#ifndef BX_IDS_CACHE
#define BX_IDS_CACHE

#include <stdbool.h>
#include <stdint.h>

typedef struct Cache Cache;

typedef struct {
  Cache *c;
  uint32_t current;
  uint64_t version;
} CacheIter;

typedef enum { CacheOk = 0, CacheNotSet, CacheNotSync } CacheState;

Cache *cache_create(void);
void cache_destroy(Cache *c);
bool cache_set_item(Cache *c, uint64_t id, uint64_t checksum);
/* Marks the id seen this cycle when it is already in the cache. */
CacheState cache_check_item(Cache *c, uint64_t id, uint64_t checksum);
void cache_next_version(Cache *c);

/**
 * Init a cache iterator. The cache iterator is set at the cache version
 * on init so any operation involving versionning will be at a stable
 * value.
 *
 * @param[in]  c    The cache pointer
 * @param[out] iter Iterator to init, it is not allocated.
 */
void cache_iter_init(Cache *c, CacheIter *iter);
/**
 * Get the next ID that is prunable (last_seen drifted away from version too
 * much). Does not mutate the item; call cache_tombstone after a successful
 * DELETE, then cache_prune to compact.
 *
 * @param[in]  iter  A cache iterator
 * @param[in]  drift The minimal drift to be considered prunable.
 * @param[out] id    The prunable id
 *
 * @return true if an id was written, false at the end
 */
bool cache_iter_next_prunable_id(CacheIter *iter, uint64_t drift, uint64_t *id);
/**
 * Mark an id as deleted (last_seen = 0). Call cache_prune to remove it.
 */
void cache_tombstone(Cache *c, uint64_t id);
/**
 * Prune the cache phyisically removing deleted value
 *
 * @param[in] c The cache object to prunable
 */
void cache_prune(Cache *c);

#endif /* BX_IDS_CACHE */
