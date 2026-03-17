#ifndef MINERS_HASHMAP_H
#define MINERS_HASHMAP_H

#include "common.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
  void *key;
  void *value;
  uint8_t state;
} hm_entry_t;

typedef struct {
  size_t count;
  size_t capacity;
  size_t tombstones;
  hm_entry_t *entries;
  size_t (*hash_fn)(const void *key);
  int (*cmp)(const void *a, const void *b);

} hm_t;

hm_t *hm_create(size_t init_size, size_t (*hash_fn)(const void *key),
                int (*cmp)(const void *a, const void *b));

void hm_destroy(hm_t *map);

int hm_put(hm_t *map, void *key, void *value);
// returns  0=ok -1=error

void *hm_get(hm_t *map, void *key);
// returns value or NULL

void hm_remove(hm_t *map, void *key);
// deletes the entry if it exists.

hm_t *hm_resize(hm_t *map, uint32_t cap);

size_t fnv1a_int(const void *key);
size_t fnv1a_str(const void *key);

#endif
