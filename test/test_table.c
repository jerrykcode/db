#include "../table.h"
#include <string.h>

static char * char_pointer(const char *str) {
    char *result = (char *)malloc(strlen(str));
    strcpy(result, str);
    return result;
}

static void map_free_all(Map *map) {
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

static void insert_1(Table *table) {
    for (int i = 0; i < 1024; i++) {
        KeyValueMap *map = new_map();
        map_put(map, char_pointer("id"), char_pointer("1000001"));
        map_put(map, char_pointer("num"), char_pointer("8888"));
        table_insert(table, map);
        map_free_all(map);
    }
}

static void insert_2(Table *table) {
    KeyValueMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("1000005"));
    map_put(map, char_pointer("num"), char_pointer("8887"));
    table_insert(table, map);
    map_free_all(map);
}



static void select_1(Table *table) {
    KeyValueMap *example = new_map();
    map_put(example, char_pointer("id"), char_pointer("1000001"));
    table_select(table, example);
    map_free_all(example);
}

static void select_2(Table *table) {
    KeyValueMap *example = new_map();
    map_put(example, char_pointer("num"), char_pointer("8887"));
    table_select(table, example);
    map_free_all(example);
}

int main() {
    KeyList *list = new_list();
    
    list_add(list, char_pointer("id"));
    list_add(list, char_pointer("num"));
    KeyTypeMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("bigint"));
    map_put(map, char_pointer("num"), char_pointer("int"));
    Table *table = table_create("./", "tmp_table", list, map);
    insert_1(table);
    table_close(table);
    table = table_open("./", "tmp_table");
    for (int i = 0; i < 9; i++) {
        insert_2(table);
        insert_1(table);
    }
    select_1(table);
    printf("\n");
    select_2(table);
    table_close(table);
    exit(0);
}
