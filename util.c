#include "util.h"

List *new_list() {
    List *list = (List *)malloc(sizeof(List));
    list->size = 0;
}

size_t list_size(List *list) {
    return list->size;
}

void list_add(List *list, char *key) {
    if (list->size < CAPACITY)
        list->arr[list->size++] = key;
}

char *list_get(List *list, size_t i) {
    return list->arr[i];
}

void list_free(List *list) {
    free(list);
}

typedef struct Pair {
    char *key;
    char *value;
};

static int cmp(const void *a, const void *b) {
    return strcmp(((struct Pair *)a)->key, ((struct Pair *)b)->key);
}

static void destroy(void *a) {
    struct Pair *pair = (struct Pair *)a;
    free(pair->key);
    free(pair->value);
    free(pair);
}

Map *new_map() {
    Map *map = (Map *)malloc(sizeof(Map));
    map->rbtree = rbtree_create(cmp, destroy, sizeof(struct Pair), RBT_SHALLOW_COPY | RBT_INSERT_REPLACE);
	return map;
}

size_t map_size(Map *map) {
    return rbtree_size(map->rbtree);
}

void map_put(Map *map, char *key, char *value) {
    struct Pair *p = malloc(sizeof(struct Pair));
    p->key = key;
    p->value = value;
    rbtree_insert(map->rbtree, p);
}

char *map_get(Map *map, const char *key) {
    struct Pair *p = malloc(sizeof(struct Pair));
    p->key = key;
    void *res = rbtree_search(map->rbtree, p);
    free(p);
    if (res) {
        return ((struct Pair *)res)->value;
    }
    return NULL;
}

void map_get_all_keys(Map *map, char **keys) {
    rbtree_iterator_t *it = rbtree_iterator_create(map->rbtree);
    size_t index = 0;
    while (1) {
        void *ptr = rbtree_iterator_current(it);
        keys[index++] = ((struct Pair *)ptr)->key;
        if (rbtree_iterator_has_next(it))
            rbtree_iterator_next(it);
        else
            break;
    }
}

void map_get_all_values(Map *map, char **values) {
    rbtree_iterator_t *it = rbtree_iterator_create(map->rbtree);
    size_t index = 0;
    while (1) {
        void *ptr = rbtree_iterator_current(it);
        values[index++] = ((struct Pair *)ptr)->value;
        if (rbtree_iterator_has_next(it))
            rbtree_iterator_next(it);
        else
            break;
    }

}

void map_get_all_keys_and_values(Map *map, char **keys, char **values) {
    rbtree_iterator_t *it = rbtree_iterator_create(map->rbtree);
    size_t index = 0;
    while (1) {
        void *ptr = rbtree_iterator_current(it);
        keys[index] = ((struct Pair *)ptr)->key;
        values[index++] = ((struct Pair *)ptr)->value;
        if (rbtree_iterator_has_next(it))
            rbtree_iterator_next(it);
        else
            break;
    }

}

void map_free(Map *map) {
    rbtree_destroy(map->rbtree);
    free(map);
}
