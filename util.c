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
