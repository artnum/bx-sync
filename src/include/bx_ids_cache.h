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

typedef enum { CacheOk = 0, CacheNotSet, CacheNotSync } CacheState;

Cache *cache_create();
void cache_print(Cache *c);
bool cache_set_item(Cache *c, BXGeneric *item_id, uint64_t checksum);
void cache_destroy(Cache *c);
CacheState cache_check_item(Cache *c, BXGeneric *item_id, uint64_t checksum);
void cache_store(Cache *c, const char *filename);
void cache_stats(Cache *c, const char *name);
#endif /* BX_IDS_CACHE */
