#ifndef BX_IDS_CACHE
#define BX_IDS_CACHE

#include "bx_object_value.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  BXAny id;
  uint64_t checksum;
} CacheItem;

typedef struct {
  uint32_t size;
  uint32_t count;
  CacheItem *items;
} Cache;

Cache *cache_create();
void cache_print(Cache *c);
bool cache_set_item(Cache *c, BXGeneric *item_id, uint64_t checksum);
#endif /* BX_IDS_CACHE */
