#ifndef BX_IDS_CACHE
#define BX_IDS_CACHE

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t id;
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

/**
 * Allocate memory for cache object.
 *
 * @return A cache object
 */
Cache *cache_create();
void cache_print(Cache *c);
bool cache_set_item(Cache *c, uint64_t id, uint64_t checksum);
void cache_destroy(Cache *c);
/* Marks the id seen this cycle when it is already in the cache. */
CacheState cache_check_item(Cache *c, uint64_t id, uint64_t checksum);

CacheItem *cache_get(Cache *c, uint32_t idx);
void cache_stats(Cache *c, const char *name);
/**
 * Invalidate (last seen to 0) all item that have drifted
 *
 * @param[in] c     Cache to Invalidate.
 * @param[in] drift Minimal drift to Invalidate.
 */
void cache_invalidate(Cache *c, uint64_t drift);
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
 * Get next id in cache.
 *
 * @param[in] A cache iterator
 *
 * @return A pointer to the ID, or NULL at the end
 */
const uint64_t *cache_iter_next_id(CacheIter *iter);

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

/**
 * Empty the cache, freeing item memory
 *
 * @param[in] c Cache to empty
 */
void cache_empty(Cache *c);
void cache_reset_version(Cache *c);

#define cache_next_version(c)                                                  \
  do {                                                                         \
    if (c) {                                                                   \
      c->version++;                                                            \
      if (c->version == UINT64_MAX) {                                          \
        cache_reset_version(c);                                                \
      }                                                                        \
    }                                                                          \
  } while (0)

#endif /* BX_IDS_CACHE */
