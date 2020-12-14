#ifndef MAP_H__
#define MAP_H__

#include <stdlib.h>
#include <stdbool.h>

typedef void map_t;

/* flags */
extern int MAP_KEY_SHALLOW_COPY;
extern int MAP_KEY_REFERENCE_COPY;
extern int MAP_VALUE_SHALLOW_COPY;
extern int MAP_VALUE_REFERENCE_COPY;

map_t *map_create(int (*compare_key)(const void *, const void *), int flags);
int map_set_key_size(map_t *, size_t key_size);
int map_set_value_size(map_t *, size_t value_size);
int map_set_destroy_key(map_t *, void (*destroy_key)(void *));
int map_set_destroy_value(map_t *, void (*destroy_value)(void *));
void map_destroy(map_t *);

void *map_get(map_t *, void *key);
int map_put(map_t *, void *key, void *value);
bool map_has_key(map_t *, void *key);
int map_remove(map_t *, void *key);
size_t map_size(map_t *);
int map_sort(map_t *, void **key_arr, void **value_arr);
/*

For MAP_KEY_SHALLOW_COPY, the map will shallow copies the key when map_put is called. The map WILL free 
    the memory of key when map_remove or map_destroy is called.
For MAP_KEY_REFERENCE_COPY, the map will shallow copies the key when map_put is called and will NOT free 
    the memory of key when map_remove or map_destroy is called.
If both MAP_KEY_SHALLOW_COPY and MAP_KEY_REFERENCE_COPY are NOT selected, the map will deep copies the key
    when map_put is called and WILL free the memory of key when map_remove or map_destroy is called.
In the cases when memory of key needed to be free, a function that can destroy a key is required. map_create 
    will set this function to a default one:
        static void default_destroy(void *ptr) {
            if (ptr == NULL)
                return;
            free(ptr);
        }
    However, memory leak may be occur if using this default function.
    map_set_destroy_key can be used to set this function to a custom function.

In the case when key is deep copied, the size of key is required. map_set_key_size can be used to set the size.


For MAP_VALUE_SHALLOW_COPY, the map will shallow copies the value when map_put is called. The map WILL free
     the memory of value when map_remove or map_destroy is called.
For MAP_VALUE_REFERENCE_COPY, the map will shallow copies the value when map_put is called and will NOT free
     the memory of value when map_remove or map_destroy is called.
If both MAP_VALUE_SHALLOW_COPY and MAP_VALUE_REFERENCE_COPY are NOT selected, the map will deep copies the value
     when map_put is called and WILL free the memory of value when map_remove or map_destroy is called.
In the cases when memory of value need to be free, a function that can destroy a value is required. map_create 
    will set this function to a default one:
        static void default_destroy(void *ptr) {
            if (ptr == NULL)
                return;
            free(ptr);
        }
    However, memory leak may be occur if using this default function.
    map_set_destroy_value can be used to set this function to a custom function.

In the case when value is deep copied, the size of value is required. map_set_value_size can be used to set the size.

*/


#endif
