#include "../table.h"
#include <string.h>

static char * char_pointer(const char *str) {
    char *result = (char *)malloc(strlen(str));
    strcpy(result, str);
    return result;
}

static void insert(Table *table) {
    KeyValueMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("1000001"));
    map_put(map, char_pointer("num"), char_pointer("8888"));
    table_insert(table, map);

    //free map
    size_t num_keys = map_size(map);
    char **keys = (char **)malloc(num_keys * sizeof(char *));
    map_get_all_keys(map, keys);
    for (int i = 0; i < num_keys; i++)
        free(keys[i]);
    free(keys);
    char **types = (char **)malloc(num_keys * sizeof(char *));
    map_get_all_values(map, types);
    for (int i = 0; i < num_keys; i++)
        free(types[i]);
    free(types);
    map_free(map);

}

int main() {
    KeyList *list = new_list();
    
    list_add(list, char_pointer("id"));
    list_add(list, char_pointer("num"));
    KeyTypeMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("bigint"));
    map_put(map, char_pointer("num"), char_pointer("int"));
    Table *table = table_create("./", "tmp_table", list, map);
    insert(table);
    table_close(table);
    table = table_open("./", "tmp_table");
    for (int i = 0; i < 9; i++) insert(table);
    table_close(table);
    exit(0);
}
