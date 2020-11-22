#include "../table.h"
#include <string.h>

static char * char_pointer(const char *str) {
    char *result = (char *)malloc(strlen(str));
    strcpy(result, str);
    return result;
}

static void map_free_all(Map *map) {
    //free map
    size_t num_cols = map_size(map);
    char **keys = (char **)malloc(num_cols * sizeof(char *));
    map_get_all_keys(map, keys);
    for (int i = 0; i < num_cols; i++)
        free(keys[i]);
    free(keys);
    char **types = (char **)malloc(num_cols * sizeof(char *));
    map_get_all_values(map, types);
    for (int i = 0; i < num_cols; i++)
        free(types[i]);
    free(types);
    map_free(map);
}

static void insert_1(Table *table, int n) {
    for (int i = 0; i < n; i++) {
        ColNameValueMap *map = new_map();
        map_put(map, char_pointer("id"), char_pointer("1000001"));
        map_put(map, char_pointer("num"), char_pointer("8888"));
        table_insert(table, map);
        map_free_all(map);
    }
}

static void insert_2(Table *table) {
    ColNameValueMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("1000005"));
    map_put(map, char_pointer("num"), char_pointer("8887"));
    table_insert(table, map);
    map_free_all(map);
}

static void select_1(Table *table) {
    ColNameValueMap *example = new_map();
    map_put(example, char_pointer("id"), char_pointer("1000001"));
    table_select(table, example);
    map_free_all(example);
}

static void select_2(Table *table) {
    ColNameValueMap *example = new_map();
    map_put(example, char_pointer("num"), char_pointer("8887"));
    table_select(table, example);
    map_free_all(example);
}

int main() {
    ColNameList *list = new_list();
    
    list_add(list, char_pointer("id"));
    list_add(list, char_pointer("num"));
    ColNameTypeMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("bigint"));
    map_put(map, char_pointer("num"), char_pointer("int"));
    Table *table = table_create("./", "tmp_table", list, map);
    insert_1(table, 1);
    table_close(table);
    table = table_open("./", "tmp_table");
    for (int i = 0; i < 9; i++) {
        insert_2(table);
        insert_1(table, 1);
    }
    select_1(table); //10 items with id = 1000001
    select_2(table); //9 items with num = 8887
    insert_1(table, 10240);
    insert_2(table);
    insert_1(table, 1024);
    select_2(table); //10 items with num = 8887
    table_close(table);
    exit(0);
}
