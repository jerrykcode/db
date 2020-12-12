#include <errno.h>
#include "map.h"
#include "rbtree.h"

int MAP_SHALLOW_COPY = 0x00000001;
int MAP_REFERENCE_COPY = 0x0000002;

typedef struct Map {
    int flags;
    int (*compare_key)(const void *, const void *);
    rbtree_t rbtree;
} *PMap;

typedef struct Pair {
    void *key;
    void *value;
    PMap map;
};

static int compare_pair(const void *a, const void *b) {
    return ((struct Pair *)a)->map->compare_key(
        ((struct Pair *)a)->key, 
        ((struct Pair *)b)->key
    );
}

map_t *map_create(int (*compare_key)(const void *, const void *), int flags) {
    PMap map = malloc(sizeof(struct Map));
    if (map == NULL)
        return NULL;
    map->flags = flags;
    map->compare_key = compare_key;
    if ((map->rbtree = rbtree_create(compare_pair, sizeof(struct Pair), RBT_SHALLOW_COPY)) == NULL) {
        free(map);
        return NULL;
    }
    return map;
}

void map_destroy(map_t *ptr) {
    if (ptr == NULL)
        return ;
    PMap map = (PMap)ptr;
    rbtree_destroy(ptr->rbtree);
    free(map);
}

void *map_get(map_t *ptr, const void *key) {
    if (ptr == NULL)
        return NULL;
    PMap map = (PMap)ptr;
    if (key == NULL && (!map->flags & MAP_REFERENCE_COPY))
        return NULL;
    struct Pair *key_pair = malloc(sizeof(struct Pair));
    if (key_pair == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    key_pair->key = key;
    key_pair->value = NULL;

    struct Pair *pair = rbtree_search(map->rbtree, key_pair);
    free(key_pair);
    if (pair == NULL)
        return NULL;
    return pair->value;
}

int map_put(map_t *ptr, const void *key, const void *value) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    if (key == NULL && (! map->flags & MAP_REFERENCE_COPY))
        return EINVAL;
    struct Pair *pair = malloc(sizeof(struct Pair));
    if (pair == NULL)
        return ENOMEM;
    pair->key = key;
    pair->value = value;
    int ret = rbtree_insert(map->rbtree, pair);
    if (ret != 0) {
        free(pair);
    }
    return ret;
}

bool map_has_key(map_t *ptr, const void *key) {
    return map_get(ptr, key) != NULL;
}

int map_remove(map_t *ptr, const void *key) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    if (key == NULL && (! map))
}

size_t map_size(map_t *) {

}
