#include "include/bx_ids_cache.h"
#include "include/bx_object.h"
#include "include/bx_object_value.h"
#include "include/bx_utils.h"
#include <stdio.h>
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
  if (c->count < 2) {
    return;
  }
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
  c->version = 1;
  return c;
}

void cache_stats(Cache *c, const char *name) {
  float cs = (float)c->count * sizeof(CacheItem) / 1024;
  bx_log_info("Cache %s : %.2f size [kb], %lu items, %lu total, %lu version",
              name, cs, c->count, c->size, c->version);
}

BXGeneric *_item_to_id(CacheItem *item) {
  switch (*(uint8_t *)item) {
  case BX_OBJECT_TYPE_INTEGER:
    return (BXGeneric *)&item->id.__int;
  case BX_OBJECT_TYPE_UINTEGER:
    return (BXGeneric *)&item->id.__uint;
  case BX_OBJECT_TYPE_FLOAT:
    return (BXGeneric *)&item->id.__float;
  case BX_OBJECT_TYPE_UUID:
    return (BXGeneric *)&item->id.__uuid;
  case BX_OBJECT_TYPE_STRING:
    return (BXGeneric *)&item->id.__string;
  case BX_OBJECT_TYPE_BYTES:
    return (BXGeneric *)&item->id.__bytes;
  case BX_OBJECT_TYPE_BOOL:
    return (BXGeneric *)&item->id.__bool;
  }
  return NULL;
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

CacheItem *cache_get_by_id(Cache *c, BXGeneric *item_id) {
  return _find_item(c, item_id);
}

CacheItem *cache_get(Cache *c, uint32_t id) {
  if (id >= c->count) {
    return NULL;
  }
  return &c->items[id];
}

void cache_iter_init(Cache *c, CacheIter *iter) {
  if (iter == NULL) {
    return;
  }
  iter->current = 0;
  iter->c = c;
  iter->version = c->version;
}

const BXGeneric *cache_iter_next_id(CacheIter *iter) {
  CacheItem *item = NULL;
  do {
    item = cache_get(iter->c, iter->current);
    if (item == NULL) {
      iter->current = 0;
      return NULL;
    }
    iter->current++;
  } while (item != NULL && item->last_seen == 0);
  return _item_to_id(item);
}

const BXGeneric *cache_iter_next_prunable_id(CacheIter *iter, uint64_t drift,
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
      return _item_to_id(item);
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
      i++;
      item->last_seen = 0;
    }
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

void cache_delete_idx(Cache *c, uint32_t idx) {
  CacheItem *item = NULL;
  item = cache_get(c, idx);
  if (item != NULL) {
    item->last_seen = 0;
  }
}

bool cache_set_item(Cache *c, BXGeneric *item_id, uint64_t checksum) {
  CacheItem *current = _find_item(c, item_id);
  bool need_sort = true;
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
    if (c->count > 1 &&
        _cmp(&c->items[c->count - 1].id, &c->items[c->count].id) <= 0) {
      need_sort = false;
    }
    c->count++;
    if (need_sort) {
      _heapsort_cache(c);
    }
    return true;
  }

  current->checksum = checksum;
  current->last_seen = c->version;
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

enum CACHE_VERSION { v1 = 1 };

void cache_store(Cache *c, const char *filename) {
  FILE *fp = NULL;
  _heapsort_cache(c);
  fp = fopen(filename, "wb");
  if (!fp) {
    return;
  }

  fseek(fp, sizeof(uint8_t) + sizeof(uint32_t), SEEK_SET);
  size_t max_entry_size = 0;
  uint8_t *entry = NULL;
  for (uint32_t i = 0; i < c->count; i++) {
    const BXGeneric *value = bx_any_to_generic(&c->items[i].id);
    if (value == NULL) {
      continue;
    }
    uint8_t type = *(uint8_t *)value;
    size_t len = 0;
    void *v = NULL;
    switch (*(uint8_t *)value) {
    case BX_OBJECT_TYPE_INTEGER:
      len = sizeof(int64_t);
      v = &((BXInteger *)value)->value;
      break;
    case BX_OBJECT_TYPE_UINTEGER:
      len = sizeof(uint64_t);
      v = &((BXUInteger *)value)->value;
      break;
    case BX_OBJECT_TYPE_FLOAT:
      len = sizeof(double);
      v = &((BXFloat *)value)->value;
      break;
    case BX_OBJECT_TYPE_UUID:
      len = sizeof(uint64_t) * 2;
      v = &((BXUuid *)value)->value;
      break;
    case BX_OBJECT_TYPE_BOOL:
      len = sizeof(bool);
      v = &((BXBool *)value)->value;
      break;
    case BX_OBJECT_TYPE_STRING:
      len = ((BXString *)value)->value_len;
      v = ((BXString *)value)->value;
      break;
    case BX_OBJECT_TYPE_BYTES:
      len = ((BXBytes *)value)->value_len;
      v = ((BXBytes *)value)->value;
      break;
    }
    if (v == NULL) {
      fclose(fp);
      return;
    }
    size_t entry_size =
        sizeof(uint8_t) + sizeof(uint64_t) + sizeof(size_t) + len;
    if (entry_size > max_entry_size) {
      void *tmp = realloc(entry, entry_size);
      if (tmp == NULL) {
        free(entry);
        fclose(fp);
        return;
      }
      entry = tmp;
      max_entry_size = entry_size;
    }

    if (entry == NULL) {
      fclose(fp);
      return;
    }

    uint8_t *ptr = entry;
    *ptr = type;
    ptr++;
    memcpy(ptr, &c->items[i].checksum, sizeof(uint64_t));
    ptr += sizeof(uint64_t);
    memcpy(ptr, &len, sizeof(size_t));
    ptr += sizeof(ptr);
    memcpy(ptr, v, len);
    fwrite(entry, entry_size, 1, fp);
  }
  fseek(fp, 0, SEEK_SET);
  uint8_t version = v1;
  fwrite(&version, sizeof(uint8_t), 1, fp);
  fwrite(&c->count, sizeof(uint32_t), 1, fp);

  free(entry);
  fclose(fp);
}

bool cache_load(Cache *c, const char *filename) {
  assert(c != NULL);
  assert(filename != NULL);
  FILE *fp = NULL;
  fp = fopen(filename, "rb");
  if (!fp) {
    return false;
  }

  uint8_t version = 0;
  fread(&version, sizeof(uint8_t), 1, fp);
  if (version != v1) {
    fclose(fp);
    return false;
  }
  uint32_t count = 0;
  fread(&count, sizeof(uint32_t), 1, fp);
  if (count == 0) {
    fclose(fp);
    return false;
  }

  if (c->items != NULL) {
    cache_empty(c);
  }

  size_t cache_mul_size = count / CACHE_CHUNK_SIZE;
  cache_mul_size++;
  CacheItem *new = calloc(sizeof(*new), cache_mul_size * CACHE_CHUNK_SIZE);
  if (!new) {
    fclose(fp);
    return false;
  }
  c->items = new;
  c->size = cache_mul_size * CACHE_CHUNK_SIZE;
  c->count = 0;

  size_t max_entry_size = 0;
  uint8_t *entry = NULL;
  const size_t entry_head_len =
      sizeof(uint8_t) + sizeof(uint64_t) + sizeof(size_t);
  uint8_t entry_head[entry_head_len];
  uint32_t i = 0;
  bool fail_read = false;
  bool first = true;
  do {
    if (fread(entry_head, entry_head_len, 1, fp) == 0) {
      fail_read = true;
      break;
    }
    c->items[i].last_seen = 1;
    memcpy(&c->items[i].checksum, &entry_head[1], sizeof(uint64_t));
    size_t len = 0;
    memcpy(&len, &entry_head[sizeof(uint8_t) + sizeof(uint64_t)],
           sizeof(size_t));
    if (len > max_entry_size) {
      void *tmp = realloc(entry, len);
      if (!tmp) {
        fail_read = true;
        break;
      }
      entry = tmp;
      max_entry_size = len;
    }
    if (fread(entry, len, 1, fp) == 0) {
      fail_read = true;
      break;
    }

    switch (entry_head[0]) {
    case BX_OBJECT_TYPE_INTEGER: {
      memcpy(&c->items[i].id.__int.value, entry, sizeof(int64_t));
      c->items[i].id.__int.isset = true;
      c->items[i].id.__int.type = entry_head[0];
    } break;
    case BX_OBJECT_TYPE_UINTEGER: {
      memcpy(&c->items[i].id.__uint.value, entry, sizeof(uint64_t));
      c->items[i].id.__uint.isset = true;
      c->items[i].id.__uint.type = entry_head[0];
    } break;
    case BX_OBJECT_TYPE_FLOAT: {
      memcpy(&c->items[i].id.__float.value, entry, sizeof(double));
      c->items[i].id.__float.isset = true;
      c->items[i].id.__float.type = entry_head[0];
    } break;
    case BX_OBJECT_TYPE_BOOL: {
      memcpy(&c->items[i].id.__bool.value, entry, sizeof(bool));
      c->items[i].id.__bool.isset = true;
      c->items[i].id.__bool.type = entry_head[0];
    } break;
    case BX_OBJECT_TYPE_UUID: {
      memcpy(&c->items[i].id.__uuid.value, entry, sizeof(uint64_t) * 2);
      ((BXUuid *)&c->items[i].id.__uuid)->isset = true;
      ((BXUuid *)&c->items[i].id.__uuid)->type = entry_head[0];
    } break;
    case BX_OBJECT_TYPE_STRING: {
      c->items[i].id.__string.value = calloc(1, len);
      if (c->items[i].id.__string.value) {
        memcpy(c->items[i].id.__string.value, entry, len);
        c->items[i].id.__string.isset = true;
        c->items[i].id.__string.type = entry_head[0];
        c->items[i].id.__string.value_len = len;
      } else {
        fail_read = true;
      }
    } break;
    case BX_OBJECT_TYPE_BYTES: {
      c->items[i].id.__bytes.value = calloc(1, len);
      if (c->items[i].id.__bytes.value) {
        memcpy(c->items[i].id.__bytes.value, entry, len);
        c->items[i].id.__bytes.isset = true;
        c->items[i].id.__bytes.type = entry_head[0];
        c->items[i].id.__bytes.value_len = len;
      } else {
        fail_read = true;
      }
      break;
    }
    }

    i++;
    first = false;
    if (i >= count) {
      break;
    }
  } while (!fail_read);

  if (fail_read && !first) {
    for (uint32_t j = 0; j < i; j++) {
      switch (*(uint8_t *)&c->items[j].id) {
      case BX_OBJECT_TYPE_BYTES:
        if (c->items[j].id.__bytes.value) {
          free(c->items[j].id.__bytes.value);
        }
      case BX_OBJECT_TYPE_STRING:
        if (c->items[j].id.__string.value) {
          free(c->items[j].id.__string.value);
        }
      }
    }
    if (c->items) {
      free(c->items);
    }
    c->size = 0;
    c->count = 0;
  }
  c->count = i;

  free(entry);
  fclose(fp);
  return true;
}
void cache_empty(Cache *c) {
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
}
void cache_destroy(Cache *c) {
  if (c == NULL) {
    return;
  }
  cache_empty(c);
  free(c);
}
