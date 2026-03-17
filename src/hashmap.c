#include "../include/hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

size_t fnv1a_int(const void *key) {
  int k = *(const int *)key;
  size_t hash = 14695981039346656037ULL;

  for (int i = 0; i < (int)sizeof(k); i++) {
    hash ^= (k >> (i * 8)) & 0xFF;
    hash *= 1099511628211ULL;
  }

  return hash;
}

size_t fnv1a_str(const void *key) {
  const char *s = (const char *)key;

  size_t hash = 14695981039346656037ULL;

  while (*s) {
    hash ^= (unsigned char)*s++;
    hash *= 1099511628211ULL;
  }

  return hash;
}

hm_t *hm_create(size_t init_size, size_t (*hash_fn)(const void *key),
                int (*cmp)(const void *a, const void *b)) {

  hm_t *Table = NULL;
  Table = (hm_t *)malloc(sizeof(hm_t));
  if (Table == NULL) {
    LOG_ERR("malloc");
    return NULL;
  }
  memset(Table, 0, sizeof(hm_t));

  Table->capacity = init_size;

  hm_entry_t *entries = NULL;
  entries = (hm_entry_t *)malloc(sizeof(hm_entry_t) * init_size);
  if (entries == NULL) {
    LOG_ERR("malloc array");
    free(Table);
    return NULL;
  }

  Table->entries = entries;
  memset(Table->entries, 0, sizeof(hm_entry_t) * init_size);

  Table->hash_fn = hash_fn;
  Table->cmp = cmp;

  return Table;
}

void hm_destroy(hm_t *map) {

  if (!map)
    return;
  if (map->entries != NULL)
    free(map->entries);
  free(map);
}

// find entries
static hm_entry_t *hm_find(hm_t *map, void *key) {

  size_t hash = map->hash_fn(key);
  size_t index = hash % map->capacity;

  hm_entry_t *entry, *tombstone = NULL;
  for (;;) {
    entry = &map->entries[index];

    if (entry->key == NULL) {
      // if entry is NULL and we already found a tombstone ,
      // then return it.
      if (entry->state == 0) {

        return (tombstone != NULL) ? tombstone : entry;

      } else {

        if (tombstone == NULL)
          tombstone = entry;
      }
    } else {
      if (map->cmp(entry->key, key) == 0) {
        return entry;
      }
    }

    index = (index + 1) % map->capacity;
  }
}

// resizes the hashmap
// returns the map pointer on successs or NULL on malloc error.
//
hm_t *hm_resize(hm_t *map, uint32_t cap) {

  hm_entry_t *newEntries = (hm_entry_t *)malloc(sizeof(hm_entry_t) * cap);

  if (!newEntries) {
    LOG_ERR("malloc");
    return NULL;
  }

  for (int i = 0; i < cap; i++) {
    newEntries[i].key = NULL;
    newEntries[i].value = NULL;
    newEntries[i].state = 0;
  }
  hm_entry_t *entry, *oldEntries;
  size_t oldCap, numEntries;

  oldEntries = map->entries;
  oldCap = map->capacity;
  numEntries = map->count;

  map->entries = newEntries;
  map->capacity = cap;
  map->count = 0;

  for (int i = 0; i < oldCap; i++) {

    entry = &oldEntries[i];

    if (entry->key == NULL)
      continue;
    hm_entry_t *dest = hm_find(map, entry->key);

    dest->key = entry->key;
    dest->value = entry->value;
    dest->state = entry->state;
    map->count++;
  }

  map->tombstones = 0;
  free(oldEntries);

  return map;
}

int hm_put(hm_t *map, void *key, void *value) {

  if (!map || !key || !value) {
    LOG_ERR("null value in func..");
    return -1;
  }

  if (map->count + 1 > map->capacity * TABLE_MAX_LOAD) {
    int cap = map->capacity << 1;
    if (hm_resize(map, cap) == NULL) {

      LOG_ERR("unable to resize table");
      return -1;
    }
  }

  hm_entry_t *entry = hm_find(map, key);

  int isNew = entry->key == NULL;
  if (isNew)
    map->count++;
  if (entry->state == 2)
    map->tombstones--;

  entry->key = key;
  entry->value = value;
  entry->state = 1;
  return 0;
}

void *hm_get(hm_t *map, void *key) {

  if (!map) {
    LOG_ERR("NULL value");
    return NULL;
  }

  hm_entry_t *entry = hm_find(map, key);
  if (entry->key != NULL && map->cmp(entry->key, key) == 0) {
    return entry->value;
  }
  return NULL;
}

void hm_remove(hm_t *map, void *key) {

  if (map->count == 0)
    return;

  hm_entry_t *Entry = hm_find(map, key);
  if (Entry->key == NULL)
    return;

  Entry->key = NULL;
  Entry->value = NULL;
  Entry->state = 2;

  map->tombstones++;
  map->count--;

  return;
}
