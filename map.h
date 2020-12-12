#ifndef MAP_H__
#define MAP_H__

typedef void map_t;

/* flags */
extern int MAP_SHALLOW_COPY;
extern int MAP_REFERENCE_COPY;

map_t *map_create(int (*compare_key)(const void *, const void *), int flags);
void map_destroy(map_t *);

void *map_get(map_t *, const void *key);
int map_put(map_t *, const void *key, const void *value);
bool map_has_key(map_t *, const void *key);
int map_remove(map_t *, const void *key);
size_t map_size(map_t *);

#endif
