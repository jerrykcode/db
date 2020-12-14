#include <errno.h>
#include <string.h>
#include "map.h"
#include "rbtree.h"

int MAP_KEY_SHALLOW_COPY = 0x00000001;
int MAP_KEY_REFERENCE_COPY = 0x00000002;
int MAP_VALUE_SHALLOW_COPY = 0x00000004;
int MAP_VALUE_REFERENCE_COPY = 0x00000008;

typedef struct Map {
    size_t key_size;
    size_t value_size;
    int flags;
    int (*compare_key)(const void *, const void *);
    void (*destroy_key)(void *);
    void (*destroy_value)(void *);
    rbtree_t *rbtree;
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

static void destroy_pair(void *ptr) {
    if (ptr == NULL)
        return;
    struct Pair *pair = (struct Pair *)ptr;
    PMap map = pair->map;
    if (!(map->flags & MAP_KEY_REFERENCE_COPY))
        map->destroy_key(pair->key);
    if (!(map->flags & MAP_KEY_REFERENCE_COPY))
        map->destroy_value(pair->value);
    free(pair);
}

static void default_destroy(void *ptr) {
    if (ptr == NULL)
        return;
    free(ptr);
}

map_t *map_create(int (*compare_key)(const void *, const void *), int flags) {
    PMap map = malloc(sizeof(struct Map));
    if (map == NULL)
        return NULL;
    map->flags = flags;
    map->compare_key = compare_key;
    map->key_size = map->value_size = 0;
    map->destroy_key = default_destroy;
    map->destroy_value = default_destroy;
    if ((map->rbtree = rbtree_create(compare_pair, destroy_pair, sizeof(struct Pair), RBT_SHALLOW_COPY | RBT_INSERT_REPLACE)) == NULL) {
        free(map);
        return NULL;
    }
    return map;
}

int map_set_key_size(map_t *ptr, size_t key_size) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    map->key_size = key_size;
    return 0;
}

int map_set_value_size(map_t *ptr, size_t value_size) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    map->value_size = value_size;
    return 0;
}

int map_set_destroy_key(map_t *ptr, void (*destroy_key)(void *)) {
    if (ptr == NULL || destroy_key == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    map->destroy_key = destroy_key;
    return 0;
}

int map_set_destroy_value(map_t *ptr, void (*destroy_value)(void *)) {
    if (ptr == NULL || destroy_value == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    map->destroy_value = destroy_value;
    return 0;
}

void map_destroy(map_t *ptr) {
    if (ptr == NULL)
        return ;
    PMap map = (PMap)ptr;
    rbtree_destroy(map->rbtree);
    free(map);
}

static bool is_key_deep_copy(PMap map) {
    return 
        (!(map->flags & MAP_KEY_REFERENCE_COPY))
            &&
        (!(map->flags & MAP_KEY_SHALLOW_COPY))
    ;
}

static bool is_value_deep_copy(PMap map) {
    return 
        (!(map->flags & MAP_VALUE_REFERENCE_COPY))
            &&
        (!(map->flags & MAP_VALUE_SHALLOW_COPY))
    ;
}

void *map_get(map_t *ptr,  void *key) {
    if (ptr == NULL)
        return NULL;
    PMap map = (PMap)ptr;
    if (key == NULL && is_key_deep_copy(map))
        return NULL;
    struct Pair *key_pair = malloc(sizeof(struct Pair));
    if (key_pair == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    key_pair->map = map;
    key_pair->key = key;
    key_pair->value = NULL;

    struct Pair *pair = rbtree_search(map->rbtree, key_pair);
    free(key_pair);
    if (pair == NULL)
        return NULL;
    return pair->value;
}

int map_put(map_t *ptr, void *key, void *value) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    if (key == NULL && is_key_deep_copy(map))
        return EINVAL;
    if (value == NULL && is_value_deep_copy(map))
        return EINVAL;
    struct Pair *pair = malloc(sizeof(struct Pair));
    if (pair == NULL)
        return ENOMEM;
    pair->map = map;
    if (is_key_deep_copy(map)) {
        if (map->key_size == 0) {
            free(pair);
            return EINVAL; 
        }
        pair->key = malloc(map->key_size);
        if (pair->key == NULL) {
            free(pair);
            return ENOMEM;
        }
        memcpy(pair->key, key, map->key_size);
    }
    else {
        pair->key = key;
    }

    if (is_value_deep_copy(map)) {
        if (map->value_size == 0) {
            if (is_key_deep_copy(map))
                free(pair->key); //Do NOT call destroy_key
            free(pair);
            return EINVAL;
        }
        pair->value = malloc(map->value_size);
        if (pair->value == NULL) {
            if (is_key_deep_copy(map))
                free(pair->key);
            free(pair);
            return ENOMEM;
        }
        memcpy(pair->value, value, map->value_size);
    }
    else {
        pair->value = value;
    }

    int ret = rbtree_insert(map->rbtree, pair);
    if (ret != 0) {
        if (is_key_deep_copy(map))
            free(pair->key);
        if (is_value_deep_copy(map))
            free(pair->value);
        free(pair);
    }
    return ret;
}

bool map_has_key(map_t *ptr, void *key) {
    return map_get(ptr, key) != NULL;
}

int map_remove(map_t *ptr, void *key) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;
    if (key == NULL && is_key_deep_copy(map))
        return EINVAL;

    struct Pair *pair = malloc(sizeof(struct Pair));
    if (pair == NULL)
        return ENOMEM;

    pair->map = map;
    pair->key = key;
    pair->value = NULL;

    int ret = rbtree_remove(map->rbtree, pair);
    free(pair);
    return ret;
}

size_t map_size(map_t *ptr) {
    if (ptr == NULL)
        return 0;
    PMap map = (PMap)ptr;
    return rbtree_size(map->rbtree);
}

int map_sort(map_t *ptr, void **key_arr, void **value_arr) {
    if (ptr == NULL)
        return EINVAL;
    PMap map = (PMap)ptr;

    rbtree_iterator_t *it = rbtree_iterator_create(map->rbtree);
    size_t index = 0;
    while (1) {
        void *p = rbtree_iterator_current(it);
        if (key_arr) key_arr[index] = ((struct Pair *)p)->key;
        if (value_arr) value_arr[index] = ((struct Pair *)p)->value;
        index++;
        if (rbtree_iterator_has_next(it))
            rbtree_iterator_next(it);
        else
            break;
    }
    rbtree_iterator_destroy(it);
    return 0;
}
