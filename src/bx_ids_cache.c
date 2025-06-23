#include "include/bx_ids_cache.h"
#include "include/bx_object.h"
#include "include/bx_object_value.h"
#include "include/bx_utils.h"
#include <iso646.h>
#include <stdlib.h>

#define CACHE_CHUNK_SIZE 1000
bool _grow_cache(Cache *c) {
  if (!c) {
    return false;
  }
  CacheItem *new =
      realloc(c->items, (c->size + CACHE_CHUNK_SIZE) * sizeof(*new));
  if (!new) {
    return false;
  }
  memset((void *)(new + c->size), 0, CACHE_CHUNK_SIZE * sizeof(*new));
  c->items = new;
  c->size += CACHE_CHUNK_SIZE;
  return true;
}

CacheItem *_find_item(Cache *c, BXGeneric *item_id) {
  int64_t left = 0;
  int64_t right = c->count - 1;
  int64_t middle = 0;
  if (c == NULL || item_id == NULL) {
    return NULL;
  }
  if (c->count <= 0) {
    return NULL;
  }
  while (left <= right) {
    middle = left + ((right - left) / 2);
    int compare = bx_object_value_compare(&c->items[middle].id, item_id);
    if (compare < 0) {
      left = middle + 1;
    } else if (compare > 0) {
      right = middle - 1;
    } else {
      return &c->items[middle];
    }
  }
  return NULL;
}

void _swap_items(CacheItem *a, CacheItem *b) {
  BXAny tmp = a->id;
  a->id = b->id;
  b->id = tmp;
  uint64_t tck = a->checksum;
  a->checksum = b->checksum;
  b->checksum = tck;
  uint64_t version = a->last_seen;
  a->last_seen = b->last_seen;
  b->last_seen = version;
}

int _cmp(BXAny *a, BXAny *b) {
  switch (*(uint8_t *)b) {
  case BX_OBJECT_TYPE_INTEGER:
    return bx_object_value_compare(a, (BXGeneric *)&b->__int);
  case BX_OBJECT_TYPE_UINTEGER:
    return bx_object_value_compare(a, (BXGeneric *)&b->__uint);
  case BX_OBJECT_TYPE_FLOAT:
    return bx_object_value_compare(a, (BXGeneric *)&b->__float);
  case BX_OBJECT_TYPE_UUID:
    return bx_object_value_compare(a, (BXGeneric *)&b->__uuid);
  case BX_OBJECT_TYPE_STRING:
    return bx_object_value_compare(a, (BXGeneric *)&b->__string);
  case BX_OBJECT_TYPE_BOOL:
    return bx_object_value_compare(a, (BXGeneric *)&b->__bool);
  case BX_OBJECT_TYPE_BYTES:
    return bx_object_value_compare(a, (BXGeneric *)&b->__bytes);
  }
  return 0;
}

void _heapify(Cache *c, uint32_t n, uint32_t i) {
  uint32_t largest = i;
  uint32_t left = 2 * i + 1;
  uint32_t right = 2 * i + 2;

  if (left < n && _cmp(&c->items[left].id, &c->items[largest].id) > 0) {
    largest = left;
  }
  if (right < n && _cmp(&c->items[right].id, &c->items[largest].id) > 0) {
    largest = right;
  }
  if (largest != i) {
    _swap_items(&c->items[i], &c->items[largest]);
    _heapify(c, n, largest);
  }
}

void _heapsort_cache(Cache *c) {
  for (int32_t i = c->count / 2 - 1; i >= 0; i--) {
    _heapify(c, c->count, i);
  }

  for (uint32_t i = c->count - 1; i > 0; i--) {
    _swap_items(&c->items[0], &c->items[i]);
    _heapify(c, i, 0);
  }
}

Cache *cache_create() {
  Cache *c = calloc(1, sizeof(*c));
  if (!c) {
    return NULL;
  }
  if (!_grow_cache(c)) {
    free(c);
    return NULL;
  }
  return c;
}

void cache_stats(Cache *c, const char *name) {

  char *min = NULL;
  char *max = NULL;
  if (c->count > 0) {
    min = bx_any_to_str(&c->items[0].id);
    max = bx_any_to_str(&c->items[c->count - 1].id);
    bx_log_info("Cache %s : %lu items, %lu total, %lu version, [%s:%s] range",
                name, c->count, c->size, c->version, min, max);
    return;
  }
  bx_log_info("Cache %s : %lu items, %lu total, %lu version, [0:0] range", name,
              c->count, c->size, c->version);
}

void cache_print(Cache *c) {
  for (uint32_t i = 0; i < c->count; i++) {
    switch (*(uint8_t *)&c->items[i].id) {
    case BX_OBJECT_TYPE_INTEGER:
      _bx_dump_any("ID", &c->items[i].id.__int, 1);
      break;
    case BX_OBJECT_TYPE_UINTEGER:
      _bx_dump_any("ID", &c->items[i].id.__uint, 1);
      break;
    case BX_OBJECT_TYPE_FLOAT:
      _bx_dump_any("ID", &c->items[i].id.__float, 1);
      break;
    case BX_OBJECT_TYPE_UUID:
      _bx_dump_any("ID", &c->items[i].id.__uuid, 1);
      break;
    case BX_OBJECT_TYPE_STRING:
      _bx_dump_any("ID", &c->items[i].id.__string, 1);
      break;
    case BX_OBJECT_TYPE_BYTES:
      _bx_dump_any("ID", &c->items[i].id.__bytes, 1);
      break;
    case BX_OBJECT_TYPE_BOOL:
      _bx_dump_any("ID", &c->items[i].id.__bool, 1);
      break;
    }
    printf("CHECKSUM %lX\n", c->items[i].checksum);
  }
}

bool cache_set_item(Cache *c, BXGeneric *item_id, uint64_t checksum) {
  CacheItem *current = _find_item(c, item_id);
  if (item_id == NULL || c == NULL) {
    return false;
  }
  if (current && current->checksum == checksum) {
    current->last_seen = c->version;
    return true;
  }
  if (current == NULL) {
    if (c->count >= c->size) {
      if (!_grow_cache(c)) {
        return false;
      }
    }
    bx_object_value_copy(&c->items[c->count].id, item_id);
    c->items[c->count].checksum = checksum;
    c->items[c->count].last_seen = c->version;
    c->count++;
    _heapsort_cache(c);
    return true;
  }

  bx_object_free_value(current);
  bx_object_value_copy(&current->id, item_id);
  current->checksum = checksum;
  current->last_seen = c->version;

  _heapsort_cache(c);
  return true;
}

CacheState cache_check_item(Cache *c, BXGeneric *item_id, uint64_t checksum) {
  CacheItem *current = _find_item(c, item_id);
  if (current == NULL) {
    return CacheNotSet;
  }
  if (current->checksum != checksum) {
    return CacheNotSync;
  }
  current->last_seen = c->version;
  return CacheOk;
}

void cache_store(Cache *c, const char *filename) {
  FILE *fp = NULL;
  _heapsort_cache(c);
  fp = fopen(filename, "w");
  if (!fp) {
    return;
  }
  for (uint32_t i = 0; i < c->count; i++) {
    char *id = bx_any_to_str(&c->items[i].id);
    fprintf(fp, "%lX:%s;", c->items[i].checksum, id);
  }
  fclose(fp);
}

void cache_destroy(Cache *c) {
  if (c == NULL) {
    return;
  }
  for (uint32_t i = 0; i < c->count; i++) {
    bx_object_free_value(&c->items[i].id);
  }
  free(c->items);
  c->items = NULL;
  c->count = 0;
  c->size = 0;
  free(c);
}
