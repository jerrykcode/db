#ifndef TABLE_H__
#define TABLE_H__

#include "disk.h"
#include "util.h"

#define FRAME_SUFFIX ".frm"
#define DATA_SUFFIX  ".dat"
#define INDEX_SUFFIX ".idx"

typedef map_t ColNameTypeMap;
typedef map_t ColNameValueMap;

typedef struct {
    ColNameList *list;
    ColNameTypeMap *map;
    map_t *index2btree;
    DISK *data;
} Table;

Table *table_create(const char *path, const char *table_name, ColNameList *list, List *indices, ColNameTypeMap *map);
Table *table_open(const char *path, const char *table_name);
void table_close(Table *table);

void table_insert(Table *table, ColNameValueMap *map);

void table_select(Table *table, ColNameValueMap *example);

#endif 
