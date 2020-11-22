#ifndef UTIL_H__
#define UTIL_H__

#include <stdlib.h>
#include <string.h>

#define ColNameList List
#define ColNameTypeMap Map
#define ColNameValueMap Map

#define CAPACITY 1024

typedef struct {
    char *arr[CAPACITY];
    size_t size;
} List;

List *new_list();

size_t list_size(List *list);

void list_add(List *list, char *key);

char *list_get(List *list, size_t i);

void list_free(List *list);

typedef struct {
    //This structure will later be replaced by a map implemented by r-b tree
    char *keys[CAPACITY];
    char *values[CAPACITY];
    size_t size;
} Map;

Map *new_map();

size_t map_size(Map *map);

void map_put(Map *map, char *key, char *value);

char *map_get(Map *map, const char *key);

void map_get_all_keys(Map *map, char **keys);
void map_get_all_values(Map *map, char **values);
void map_get_all_keys_and_values(Map *map, char **keys, char **values);

void map_free(Map *map);
#endif
