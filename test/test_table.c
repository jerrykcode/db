#include "../table.h"
#include <string.h>

static char * char_pointer(const char *str) {
    char *result = (char *)malloc(strlen(str) + 1);
    strcpy(result, str);
    return result;
}

static void map_free_all(map_t *map) {
    //free map
    map_destroy(map);
}

static int cmp(const void *a, const void *b){
    return strcmp((const char *)a, (const char *)b);
}

static char *itoa(long long i) {
    size_t len = 0;
    long long j = i;
    while (j) {
        j /= 10;
        len++;
    }
    char *str = malloc((len + 1) * sizeof(char));
    str[len] = '\0';
    while (i) {
        str[--len] = i % 10 + '0';
        i /= 10;
    }
    return str;
}

static void insert_1(Table *table, int n) {
    for (int i = 0; i < n; i++) {
        ColNameValueMap *map = map_create(cmp, MAP_KEY_SHALLOW_COPY | MAP_VALUE_SHALLOW_COPY);
        map_put(map, char_pointer("id"), itoa(1000001 + i));
        map_put(map, char_pointer("num"), itoa(8888 + i));
        table_insert(table, map);
        map_free_all(map);
    }
}

static void insert_2(Table *table) {
    ColNameValueMap *map = map_create(cmp, MAP_KEY_SHALLOW_COPY | MAP_VALUE_SHALLOW_COPY);
    map_put(map, char_pointer("id"), char_pointer("1000005"));
    map_put(map, char_pointer("num"), char_pointer("8887"));
    table_insert(table, map);
    map_free_all(map);
}

static void select_1(Table *table) {
    ColNameValueMap *example = map_create(cmp, MAP_KEY_SHALLOW_COPY | MAP_VALUE_SHALLOW_COPY);
    map_put(example, char_pointer("id"), char_pointer("1000001"));
    table_select(table, example);
    map_free_all(example);
}

static void select_2(Table *table) {
    ColNameValueMap *example = map_create(cmp, MAP_KEY_SHALLOW_COPY | MAP_VALUE_SHALLOW_COPY);
    map_put(example, char_pointer("num"), char_pointer("8887"));
    table_select(table, example);
    map_free_all(example);
}

int main() {
    ColNameList *list = new_list();

    char *id = char_pointer("id");
    char *num = char_pointer("num");
    list_add(list, id);
    list_add(list, num);
    ColNameValueMap *map = map_create(cmp, MAP_KEY_REFERENCE_COPY | MAP_VALUE_SHALLOW_COPY);
    map_put(map, id, char_pointer("bigint"));
    map_put(map, num, char_pointer("int"));

    List *indices = new_list();
    list_add(indices, id);
    list_add(indices, num);
    
    Table *table = table_create("./", "tmp_table", list, indices, map);
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
