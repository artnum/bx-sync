#include "../src/include/bx_ids_cache.h"
#include "../src/include/bx_object_value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Missing cache file argument\n");
    exit(EXIT_FAILURE);
  }

  Cache *c = NULL;
  c = cache_create();
  if (c == NULL) {
    fprintf(stderr, "Cannot create cache\n");
    exit(EXIT_FAILURE);
  }

  if (!cache_load(c, argv[1])) {
    fprintf(stderr, "Cannot load cache file\n");
    cache_destroy(c);
    exit(EXIT_FAILURE);
  }

  CacheItem *item = NULL;
  for (uint32_t i = 0; (item = cache_get(c, i)) != NULL; i++) {
    char *value = bx_object_value_to_string((BXGeneric *)&item->id);
    printf("Cache item #%05u: %s [%lX]\n", i, value, item->checksum);
    free(value);
  }

  cache_destroy(c);
  return EXIT_SUCCESS;
}
