#ifndef BTREE_H__
#define BTREE_H__

#include "disk.h"
#include "datatype.h"
#include "vector.h"

typedef struct BTree/*Index*/ {
    DISK *disk;
    DataType *p_key_type;
    disk_pointer root;
} *PBTree;

typedef disk_pointer record_t;

PBTree btree_create(const char *path, const char *table_name, const char *idx_col_name, DataType *p_key_type);
PBTree btree_open(const char *path, const char *table_name, const char *idx_col_name, DataType *p_key_type);
void btree_close(PBTree btree);
int btree_insert(PBTree btree, void *key, record_t record);
vector_t *btree_select(PBTree btree, const void *key_start, const void *key_end);
//remove

#endif
