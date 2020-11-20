#include "../table.h"
#include <string.h>

static char * char_pointer(const char *str) {
    char *result = (char *)malloc(strlen(str));
    strcpy(result, str);
    return result;
}

int main() {
    KeyList *list = new_list();
    
    list_add(list, char_pointer("id"));
    list_add(list, char_pointer("num"));
    KeyTypeMap *map = new_map();
    map_put(map, char_pointer("id"), char_pointer("bigint"));
    map_put(map, char_pointer("num"), char_pointer("int"));
    Table *table = table_create("./", "tmp_table", list, map);
    table_close(table);
    table = table_open("./", "tmp_table");
    table_close(table);
    exit(0);
}
