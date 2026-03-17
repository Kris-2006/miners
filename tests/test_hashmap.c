#include "../include/hashmap.h"
#include <assert.h>
#include <string.h>

int cmp(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

int main() {

  hm_t *map;
  map = hm_create(32, fnv1a_str, cmp);

  assert(map->count == 0);
  hm_put(map, "tada", "love");
  assert(strcmp((const char *)hm_get(map, "tada"), "love") == 0);
  assert(map->count == 1);
  hm_put(map, "tada", "you");
  assert(strcmp((const char *)hm_get(map, "tada"), "you") == 0);
  hm_remove(map, "tada");
  assert(hm_get(map, "tada") == NULL);
  assert(map->tombstones == 1);
  assert(map->count == 0);

  hm_destroy(map);
  return 0;
}
