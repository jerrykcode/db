#include "btree.h"
#include "table.h"
#include "string.h"
#include "errno.h"

#define NODE_MAX_DEGREE 4

struct Cell {
    disk_pointer pointer;
    void *key;
}__attribute__((packed));

struct Node {
    bool flag_is_leaf;
    size_t num;
    struct Cell cells[NODE_MAX_DEGREE];
    disk_pointer last_pointer;
}__attribute__((packed));
typedef struct Node *PNode;

static char *get_disk_pathname(const char *path, const char *table_name, const char *idx_col_name) {
    size_t path_len = strlen(path);
    size_t table_name_len = strlen(table_name);
    size_t idx_col_name_len = strlen(idx_col_name);
    size_t idx_suffix_len = strlen(INDEX_SUFFIX);
    char *disk_path = malloc((path_len + table_name_len + 1 + idx_col_name_len + idx_suffix_len) * sizeof(char));
    strcpy(disk_path, path);
    strcpy(disk_path + path_len, table_name);
    disk_path[path_len + table_name_len] = '_';
    strcpy(disk_path + path_len + table_name_len + 1, idx_col_name);
    strcpy(disk_path + path_len + table_name_len + 1 + idx_col_name_len, INDEX_SUFFIX);
    return disk_path;
}

PBTree btree_create(const char *path, const char *table_name, const char *idx_col_name, int (*compare)(const void *, const void *)) {
    PBTree btree = malloc(sizeof(struct BTree));
    if (btree == NULL)
        return NULL;
    btree->disk = dcreate(get_disk_pathname(path, table_name, idx_col_name), sizeof(struct Node));
    if (btree->disk == NULL) {
        free(btree);
        return NULL;
    }
    btree->compare = compare;
    PNode node = malloc(sizeof(struct Node));
    if (node == NULL) {
        dclose(btree->disk);
        free(btree);
        return NULL;
    }
    node->flag_is_leaf = true;
    node->num = 0;
    if (dalloc_first_block(btree->disk) != 0) {
        dclose(btree->disk);
        free(btree);
        return NULL;
    }
    btree->root = first_block(btree->disk);
    if (btree->root == DNULL) {
        dclose(btree->disk);
        free(btree);
        free(node);
        return NULL;
    }
    copy_to_disk(node, sizeof(struct Node), btree->disk, btree->root);
    free(node);
    return btree;
}

PBTree btree_open(const char *path, const char *table_name, const char *idx_col_name, int (*compare)(const void *, const void *)) {
    PBTree btree = malloc(sizeof(struct BTree));
    if (btree == NULL)
        return NULL;
    btree->disk = dopen(get_disk_pathname(path, table_name, idx_col_name));
    if (btree->disk == NULL) {
        free(btree);
        return NULL;
    }
    btree->compare = compare;
    btree->root = first_block(btree->disk);
    return btree;
}

void btree_close(PBTree btree) {
    dclose(btree->disk);
    free(btree);
}

static PNode cell_insert(PNode node, int pos, disk_pointer pointer, void *key) {
    if (node->num < NODE_MAX_DEGREE) {
        for (int i = node->num - 1; i >= pos; i--) {
            node->cells[i + 1] = node->cells[i];
        }
        node->cells[pos].pointer = pointer;
        node->cells[pos].key = key;
        return NULL;
    }
    //Split into two nodes
    PNode split = (PNode)malloc(sizeof(struct Node)); //new node
    if (split == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    split->flag_is_leaf = node->flag_is_leaf;
    size_t num = node->num;
    node->num = num / 2 + 1; //node->num decrease => remove cells
    split->num = num - node->num;
    int split_start = node->num;
    if (pos < node->num) {
        node->num++;
        for (int i = split_start, j = 0; i < num; i++, j++) //copy to new node
            split->cells[j] = node->cells[i];
        for (int i = node->num - 1; i >= pos; i--)
            node->cells[i + 1] = node->cells[i];
        node->cells[pos].pointer = pointer;
        node->cells[pos].key = key;
    }
    else {
        split->num++;
        int j = 0, i;
        for (i = split_start; i < pos; i++, j++) //copy to new node
            split->cells[j] = node->cells[i];
        split->cells[j].pointer = pointer;
        split->cells[j].key = key;
        j++;
        for (i = pos; i < num; i++, j++) //copy to new node
            split->cells[j] = node->cells[i];
    }
    split->last_pointer = node->last_pointer; //copy last_pointer
    return split;
}

//insert recursively
struct Split_res {
    void *key;
    disk_pointer pointer;
};

static struct Split_res *btree_insert_re(PBTree btree, disk_pointer disk_node, void *key, record_t record) {
    DISK *disk = btree->disk;
    PNode node, split;
    int left, right, mid, compare_res;
    int pos;
    struct Split_res *res = NULL;
    disk_pointer next, tmp_pointer;

    void *buffer = malloc(disk->block_size); //disk->block_size equals to sizeof(struct Node)
    if (buffer == NULL) {
        errno = ENOMEM;
        goto ERR;
    }
    copy_to_memory(disk, disk_node, buffer); //read to memory
    node = (PNode)buffer;
    //binary search
    left = 0;
    right = node->num - 1;
    pos = node->num;
    while (left <= right) {
        mid = (left + right) / 2;
        compare_res = btree->compare(key, node->cells[mid].key);
        if (compare_res < 0) {
            right = mid - 1;
            pos = mid;
        }
        else if (compare_res > 0) {
            left = mid + 1;
        }
        else { //compare_res == 0
            pos = mid + 1;
            break;
        }
    } //while

    if (node->flag_is_leaf) {
        split = cell_insert(node, pos, record, key);
        if (errno)
            goto ERR;
        if (split == NULL)
            copy_to_disk(node, sizeof(node), disk, disk_node); //write back to disk
        if (split) {
            res = (struct Split_res *)malloc(sizeof(struct Split_res));
            if (res == NULL) {
                errno = ENOMEM;
                goto ERR;
            }
            tmp_pointer = dalloc(disk); //new node
            node->last_pointer = tmp_pointer; //'last_pointer' of leaf node points to the next node
            copy_to_disk(split, sizeof(*split), disk, tmp_pointer); //write new node to disk
            free(split);
            res->pointer = tmp_pointer;
            res->key = node->cells[node->num - 1].key;
            if (disk_node != btree->root) {
                copy_to_disk(node, sizeof(*node), disk, disk_node); //write back to disk
            }
            else {
                tmp_pointer = dalloc(disk); //Allocs another disk space
                copy_to_disk(node, sizeof(*node), disk, tmp_pointer);
                node = (PNode)malloc(sizeof(struct Node)); //Allocs new memory, the original 'node' will be free by free(buffer) at END
                if (node == NULL) {
                    errno = ENOMEM;
                    free(res);
                    goto ERR;
                }
                node->flag_is_leaf = false;
                node->num = 1;
                node->cells[0].pointer = tmp_pointer;
                node->cells[0].key = res->key;
                node->last_pointer = res->pointer;
                copy_to_disk(node, sizeof(*node), disk, btree->root); //btree->root remains unchanged
                free(node);
                free(res);
                res = NULL;
            }
            goto END;
        }
    }

    else { //Not leaf node
        next = pos < node->num ? node->cells[pos].pointer : node->last_pointer;
        res = btree_insert_re(btree, next, key, record);
        if (errno)
            goto ERR;
        if (res) {
            if (pos < node->num) {
                tmp_pointer = node->cells[pos].pointer;
                node->cells[pos].pointer = res->pointer;
            }
            else {
                tmp_pointer = node->last_pointer;
                node->last_pointer = res->pointer;
            }
            split = cell_insert(node, pos, tmp_pointer, res->key);
            if (errno)
                goto ERR;
            if (split == NULL) {
                copy_to_disk(node, sizeof(*node), disk, disk_node); //write back to disk
                free(res);
                res = NULL;
            }
            if (split) {
                node->last_pointer = node->cells[node->num - 1].pointer;
                tmp_pointer = dalloc(disk);
                res->pointer = tmp_pointer;
                res->key = node->cells[node->num - 1].key;
                node->num--;
                
                copy_to_disk(split, sizeof(*split), disk, tmp_pointer); //write mew node to disk
                free(split);
                
                if (disk_node != btree->root) {
                    copy_to_disk(node, sizeof(*node), disk, disk_node); //write back to disk
                }
                else {
                    tmp_pointer = dalloc(disk); //Allocs another disk space
                    copy_to_disk(node, sizeof(*node), disk, tmp_pointer);
                    node = (PNode)malloc(sizeof(struct Node)); //Allocs new memory, the original 'node' will be free by free(buffer) at END
                    if (node == NULL) {
                        errno = ENOMEM;
                        free(res);
                        goto ERR;
                    }
                    node->flag_is_leaf = false;
                    node->num = 1;
                    node->cells[0].pointer = tmp_pointer;
                    node->cells[0].key = res->key;
                    node->last_pointer = res->pointer; 
                    copy_to_disk(node, sizeof(*node), disk, btree->root); //btree->root remains unchanged
                    free(node);
                    free(res);
                    res = NULL;
                }
            }
        }
    }

END:
ERR:
    if (buffer)
        free(buffer);
    if (errno)
        return NULL;

    return res;
}

int btree_insert(PBTree btree, void *key, record_t record) {
    struct Split_res *res;
    PNode node;

    if (btree == NULL || key == NULL) {
        errno = EINVAL;
        goto ERR;
    }

    errno = 0;
    btree_insert_re(btree, btree->root, key, record);
    if (errno)
        goto ERR;
    
ERR:
    return errno;
}

record_t btree_select(PBTree btree, const void *key) {
    DISK *disk = btree->disk;
    void *buffer = malloc(disk->block_size);
    if (buffer == NULL)
        return DNULL;
    disk_pointer disk_node = btree->root;
    PNode node = NULL;
    disk_pointer res = DNULL;
    while (1) {
        copy_to_memory(disk, disk_node, buffer);
        node = (PNode)buffer;
        //binary search
        int left = 0, right = node->num - 1, mid;
        int pos = -1;
        while (left <= right) {
            mid = (left + right) / 2;
            int compare_res = btree->compare(key, node->cells[mid].key);
            if (compare_res < 0) {
                right = mid - 1;
                if (!node->flag_is_leaf) pos = mid;
            }
            else if (compare_res > 0) {
                left = mid + 1;
            }
            else { //compare_res == 0
                if (node->flag_is_leaf) {
                    res = node->cells[mid].pointer;
                    break;
                }
                //else
                pos = mid + 1;
                break;
            }
        } //while
        if (node->flag_is_leaf) 
            break;
        if (pos != -1)
            disk_node = node->cells[pos].pointer;
        else //key larger than all the cells
            disk_node = node->last_pointer;
    } //while
    free(buffer);
    return res;
}
