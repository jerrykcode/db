#ifndef TABLE_H__
#define TABLE_H__

#include "disk.h"
#include "util.h"

#define FRAME_SUFFIX ".frm"
#define DATA_SUFFIX  ".dat"
#define INDEX_SUFFIX ".idx"

typedef struct {
    KeyList *list;
    KeyTypeMap *map;
    DISK *data;
} Table;

Table *table_create(const char *path, const char *table_name, KeyList *list, KeyTypeMap *map);
Table *table_open(const char *path, const char *table_name);
void table_close(Table *table);

void insert(Table *table, KeyValueMap *map);

#endif 
