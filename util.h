#ifndef UTIL_H__
#define UTIL_H__

#include <stdlib.h>
#include <string.h>
#include "map.h"

#define ColNameList List

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

#endif
