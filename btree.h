#ifndef BTREE_H__
#define BTREE_H__

#include "disk.h"

typedef struct BTree/*Index*/ {
    DISK *disk;
    int (*compare)(const void *, const void *);
    disk_pointer root;
} *PBTree;

typedef disk_pointer record_t;

PBTree btree_create(const char *path, const char *table_name, const char *idx_col_name, int (*compare)(const void *, const void *));
PBTree btree_open(const char *path, const char *table_name, const char *idx_col_name, int (*compare)(const void *, const void *));
void btree_close(PBTree btree);
int btree_insert(PBTree btree, void *key, record_t record);
record_t btree_select(PBTree btree, const void *key);
//remove

#endif
