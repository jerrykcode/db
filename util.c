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

Map *new_map() {
    Map *map = (Map *)malloc(sizeof(Map));
    map->size = 0;
}

size_t map_size(Map *map) {
    return map->size;
}

void map_put(Map *map, char *key, char *value) {
    if (map->size < CAPACITY) {
        map->keys[map->size] = key;
        map->values[map->size++] = value;
    }
}

char *map_get(Map *map, const char *key) {
    for (int i = 0; i < map_size(map); i++) {
        if (strcmp(map->keys[i], key) == 0) 
            return map->values[i];
    }
    return NULL;
}

void map_get_all_keys(Map *map, char **keys) {
    for (int i = 0; i < map_size(map); i++)
        keys[i] = map->keys[i];
}

void map_get_all_values(Map *map, char **values) {
    for (int i = 0; i < map_size(map); i++)
        values[i] = map->values[i];
}

void map_get_all_keys_and_values(Map *map, char **keys, char **values) {
    for (int i = 0; i < map_size(map); i++) {
        keys[i] = map->keys[i];
        values[i] = map->values[i];        
    }
}

void map_free(Map *map) {
    free(map);
}
