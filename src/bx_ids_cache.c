#include "include/bx_ids_cache.h"
#include "include/bx_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_CHUNK_SIZE 1000

static bool _grow_cache(Cache *c) {
  if (!c) {
    return false;
  }
  CacheItem *new =
      realloc(c->items, (c->size + CACHE_CHUNK_SIZE) * sizeof(*new));
  if (!new) {
    return false;
  }
  memset(new + c->size, 0, CACHE_CHUNK_SIZE * sizeof(*new));
  c->items = new;
  c->size += CACHE_CHUNK_SIZE;
  return true;
}

/* First index with id >= key, or c->count if all ids are smaller. */
static uint32_t _lower_bound(const Cache *c, uint64_t id) {
  uint32_t left = 0;
  uint32_t right = c->count;
  while (left < right) {
    uint32_t middle = left + ((right - left) / 2);
    if (c->items[middle].id < id) {
      left = middle + 1;
    } else {
      right = middle;
    }
  }
  return left;
}

static CacheItem *_find_item(Cache *c, uint64_t id) {
  if (c == NULL || c->count == 0) {
    return NULL;
  }
  uint32_t i = _lower_bound(c, id);
  if (i < c->count && c->items[i].id == id) {
    return &c->items[i];
  }
  return NULL;
}

Cache *cache_create() {
  Cache *c = calloc(1, sizeof(*c));
  if (!c) {
    return NULL;
  }
  c->version = 1;
  return c;
}

void cache_stats(Cache *c, const char *name) {
#ifndef NO_LOG
  float cs = (float)c->count * sizeof(CacheItem) / 1024;
  bx_log_info("Cache %s : %.2f size [kb], %lu items, %lu total, %lu version",
              name, cs, c->count, c->size, c->version);
#endif
}

void cache_print(Cache *c) {
  if (c == NULL) {
    return;
  }
  for (uint32_t i = 0; i < c->count; i++) {
    printf("ID %lu CHECKSUM %lX\n", (unsigned long)c->items[i].id,
           (unsigned long)c->items[i].checksum);
  }
}

CacheItem *cache_get(Cache *c, uint32_t idx) {
  if (c == NULL || idx >= c->count) {
    return NULL;
  }
  return &c->items[idx];
}

void cache_iter_init(Cache *c, CacheIter *iter) {
  if (iter == NULL) {
    return;
  }
  iter->current = 0;
  iter->c = c;
  iter->version = c ? c->version : 0;
}

const uint64_t *cache_iter_next_id(CacheIter *iter) {
  CacheItem *item = NULL;
  do {
    item = cache_get(iter->c, iter->current);
    if (item == NULL) {
      iter->current = 0;
      return NULL;
    }
    iter->current++;
  } while (item->last_seen == 0);
  return &item->id;
}

const uint64_t *cache_iter_next_prunable_id(CacheIter *iter, uint64_t drift,
                                            bool del) {
  CacheItem *item = NULL;
  if (iter->version <= drift) {
    return NULL;
  }
  while ((item = cache_get(iter->c, iter->current)) != NULL) {
    if (item->last_seen > 0 && item->last_seen <= iter->version - drift) {
      iter->current++;
      if (del) {
        item->last_seen = 0;
      }
      return &item->id;
    }
    iter->current++;
  }

  return NULL;
}

void cache_invalidate(Cache *c, uint64_t drift) {
  CacheItem *item = NULL;
  if (c->version <= drift) {
    return;
  }
  uint32_t i = 0;
  while ((item = cache_get(c, i)) != NULL) {
    if (item->last_seen > 0 && item->last_seen <= c->version - drift) {
      item->last_seen = 0;
    }
    i++;
  }
}

void cache_prune(Cache *c) {
  uint32_t j = 0;
  for (uint32_t i = 0; i < c->count; i++) {
    if (c->items[i].last_seen != 0) {
      c->items[j] = c->items[i];
      j++;
    }
  }
  if (j < c->count) {
    memset(&c->items[j], 0, (c->count - j) * sizeof(CacheItem));
  }
  c->count = j;
}

bool cache_set_item(Cache *c, uint64_t id, uint64_t checksum) {
  if (c == NULL) {
    return false;
  }
  uint32_t i = _lower_bound(c, id);
  if (i < c->count && c->items[i].id == id) {
    c->items[i].checksum = checksum;
    c->items[i].last_seen = c->version;
    return true;
  }
  if (c->count >= c->size) {
    if (!_grow_cache(c)) {
      return false;
    }
  }
  if (i < c->count) {
    memmove(&c->items[i + 1], &c->items[i],
            (c->count - i) * sizeof(*c->items));
  }
  c->items[i].id = id;
  c->items[i].checksum = checksum;
  c->items[i].last_seen = c->version;
  c->count++;
  return true;
}

CacheState cache_check_item(Cache *c, uint64_t id, uint64_t checksum) {
  CacheItem *current = _find_item(c, id);
  if (current == NULL) {
    return CacheNotSet;
  }
  if (current->checksum != checksum) {
    return CacheNotSync;
  }
  current->last_seen = c->version;
  return CacheOk;
}

void cache_empty(Cache *c) {
  if (c == NULL) {
    return;
  }
  free(c->items);
  c->items = NULL;
  c->count = 0;
  c->size = 0;
}

void cache_destroy(Cache *c) {
  if (c == NULL) {
    return;
  }
  cache_empty(c);
  free(c);
}

void cache_reset_version(Cache *c) {
  for (uint32_t i = 0; i < c->count; i++) {
    c->items[i].last_seen = 1;
  }
  c->version = 1;
}
