#include "btree.h"
#include "table.h"
#include "string.h"
#include <stdint.h>
#include "errno.h"

#define NODE_MAX_DEGREE 100 

struct Node {
    bool flag_is_leaf;
    size_t num;
    disk_pointer pointers[NODE_MAX_DEGREE];
    disk_pointer last_pointer;
    uint8_t opt[NODE_MAX_DEGREE + 1]; //bitmap for options
    char key_data[1];
}__attribute__((packed));
typedef struct Node *PNode;

static const uint8_t EMPTY_KEY = 0x01;

static size_t get_node_size(size_t key_type_size) {
    return sizeof(struct Node) + (NODE_MAX_DEGREE + 1) * key_type_size;
}

static PNode node_create(size_t key_type_size) {
    PNode node = malloc(get_node_size(key_type_size));
    if (node == NULL)
        return NULL;
    node->key_data[NODE_MAX_DEGREE * key_type_size] = '\0';
    return node;
}

static char *get_disk_pathname(const char *path, const char *table_name, const char *idx_col_name) {
    size_t path_len = strlen(path);
    size_t table_name_len = strlen(table_name);
    size_t idx_col_name_len = strlen(idx_col_name);
    size_t idx_suffix_len = strlen(INDEX_SUFFIX);
    size_t EOF_SIZE = 1; // space for '\0'
    char *disk_path = malloc((path_len + table_name_len + 1 + idx_col_name_len + idx_suffix_len + EOF_SIZE) * sizeof(char));
    strcpy(disk_path, path);
    strcpy(disk_path + path_len, table_name);
    disk_path[path_len + table_name_len] = '_';
    strcpy(disk_path + path_len + table_name_len + 1, idx_col_name);
    strcpy(disk_path + path_len + table_name_len + 1 + idx_col_name_len, INDEX_SUFFIX);
    return disk_path;
}

PBTree btree_create(const char *path, const char *table_name, const char *idx_col_name, DataType *p_key_type) {
    PBTree btree = malloc(sizeof(struct BTree));
    if (btree == NULL)
        return NULL;
    btree->p_key_type = p_key_type;
    size_t node_size = get_node_size(p_key_type->get_type_size());
    btree->disk = dcreate(get_disk_pathname(path, table_name, idx_col_name), node_size);
    if (btree->disk == NULL) {
        free(btree);
        return NULL;
    }
    PNode node = node_create(p_key_type->get_type_size());
    if (node == NULL) {
        dclose(btree->disk);
        free(btree);
        return NULL;
    }
    node->flag_is_leaf = true;
    node->num = 0;
    node->last_pointer = DNULL;
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
    copy_to_disk(node, node_size, btree->disk, btree->root);
    free(node);
    return btree;
}

PBTree btree_open(const char *path, const char *table_name, const char *idx_col_name, DataType *p_key_type) {
    PBTree btree = malloc(sizeof(struct BTree));
    if (btree == NULL)
        return NULL;
    btree->disk = dopen(get_disk_pathname(path, table_name, idx_col_name));
    if (btree->disk == NULL) {
        free(btree);
        return NULL;
    }
    btree->p_key_type = p_key_type;
    btree->root = first_block(btree->disk);
    return btree;
}

void btree_close(PBTree btree) {
    dclose(btree->disk);
    free(btree);
}

static PNode cell_insert(PNode node, int pos, disk_pointer pointer, void *key, size_t key_type_size) {
    if (node->num < NODE_MAX_DEGREE) {
        if (pos <= node->num - 1) {
            //make room for pointer and key
            memmove(&node->pointers[pos + 1], &node->pointers[pos], (node->num - pos) * sizeof(disk_pointer));
            memmove(node->key_data + (pos + 1) * key_type_size, node->key_data + pos * key_type_size, (node->num - pos) * key_type_size);
        }
        //insert pointer
        if (node->flag_is_leaf || pos <= node->num - 1)
            node->pointers[pos] = pointer;
        else {
            node->pointers[node->num] = node->last_pointer;
            node->last_pointer = pointer;
        }
        //insert key
        if (key != NULL) {
            memcpy(node->key_data + pos * key_type_size, key, key_type_size);
            node->opt[pos] = 0;
        }
        else
            node->opt[pos] = EMPTY_KEY;
        node->num++;
        return NULL;
    }
    //Split into two nodes
    PNode split = node_create(key_type_size); //new node
    if (split == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    split->flag_is_leaf = node->flag_is_leaf;
    size_t num = node->num;
    node->num = num / 2 + 1; //node->num decrease => remove cells
    split->num = num - node->num;
    int split_start = node->num;
    split->last_pointer = node->last_pointer; //copy last_pointer
    if (pos < node->num) {
        //copy to split
        memcpy(split->pointers, &node->pointers[split_start], (num - split_start) * sizeof(disk_pointer));
        /*
        Non-leaf node have one more key at the begin
        */
        memcpy(split->key_data, node->key_data + split_start * key_type_size, (num - split_start + (node->flag_is_leaf ? 0 : 1)) * key_type_size);
        //make room for key in node
        memmove(&node->pointers[pos + 1], &node->pointers[pos], (node->num - pos) * sizeof(disk_pointer));
        memmove(node->key_data + (pos + 1) * key_type_size, node->key_data + pos * key_type_size, (node->num - pos) * key_type_size);
        //insert pointer and key into node
        node->pointers[pos] = pointer;
        if (key != NULL) {
            memcpy(node->key_data + pos * key_type_size, key, key_type_size);
            node->opt[pos] = 0;
        }
        else
            node->opt[pos] = EMPTY_KEY;
        node->num++;
    }
    else {
        if (pos > split_start) {
            //copy from [split_start, pos) of node to split
            memcpy(split->pointers, &node->pointers[split_start], (pos - split_start) * sizeof(disk_pointer));
            memcpy(split->key_data, node->key_data + split_start * key_type_size, (pos - split_start) * key_type_size);
        }
        //insert pointer and key into split
        if (node->flag_is_leaf || pos < num)
            split->pointers[pos - split_start] = pointer;
        else {
            split->pointers[pos - split_start] = split->last_pointer;
            split->last_pointer = pointer;
        }
        if (key != NULL) {
            memcpy(split->key_data + (pos - split_start) * key_type_size, key, key_type_size);
            split->opt[pos - split_start] = 0;
        }
        else
            split->opt[pos - split_start] = EMPTY_KEY;
        //copy the rest to split
        memcpy(&split->pointers[pos - split_start + 1], &node->pointers[pos], (num - pos) * sizeof(disk_pointer));
        /*
        Non-leaf node have one more key at the begin
        */ 
        memcpy(split->key_data + (pos - split_start + 1) * key_type_size, node->key_data + pos * key_type_size, (num - pos + (node->flag_is_leaf ? 0 : 1)) * key_type_size);
        
        split->num++;
    }
    return split;
}

//insert recursively

//utility structure
struct Split_res {
    void *key;
    disk_pointer pointer;
};

//utility functions
static void *first_new_key(PBTree btree, PNode node, PNode split, size_t key_type_size) {
    //look for the first new key in 'split'
    for (int i = 0; i < split->num; i++)
        if (btree->p_key_type->compare(
            split->key_data + i * key_type_size,
            node->key_data + (node->num - 1) * key_type_size
            ) != 0)
        {
            return split->key_data + i * key_type_size; //first new key            
        }
    return NULL;
}

static void *first_nonempty_key(PNode node, size_t key_type_size) {
    for (int i = 0; i <= node->num; i++) 
        if (! (node->opt[i] & EMPTY_KEY))
            return node->key_data + i * key_type_size;
    return NULL;
}

//insert recursively
static struct Split_res *btree_insert_re(PBTree btree, disk_pointer disk_node, void *key, record_t record) {
    DISK *disk = btree->disk;
    PNode node, split;
    void *p_key_data;
    size_t key_type_size = btree->p_key_type->get_type_size();
    int left, right, mid, compare_res;
    int pos;
    int i; //use by for loop
    struct Split_res *res = NULL;
    disk_pointer next, tmp_pointer;

    void *buffer = malloc(disk->block_size); //disk->block_size equals to get_node_size(btree->p_key_type->type_size())
    if (buffer == NULL) {
        errno = ENOMEM;
        goto ERR;
    }
    copy_to_memory(disk, disk_node, buffer); //read to memory
    node = (PNode)buffer;
    p_key_data = node->key_data;

    if (node->flag_is_leaf) {
        pos = 0;
        for (i = 0; i < node->num; i++) {
            compare_res = btree->p_key_type->compare(key, p_key_data + i * key_type_size);
            if (compare_res > 0)
                pos++;
            else if (compare_res == 0) {
                pos++;
                break;
            }
            else //compare_res < 0
                break;
        }
    }
    else {
        pos = -1;
        for (i = 0; i <= node->num; i++) {
            if (node->opt[i] & EMPTY_KEY)
                continue;
            compare_res = btree->p_key_type->compare(key, p_key_data + i * key_type_size);
            if (compare_res > 0)
                pos = i;
            else if (compare_res == 0) {
                pos = i;
                break;
            }
            else //compare_res < 0
                break;
        }
        if (pos == -1) {
            //'key' smaller than all the keys in node
            memcpy(p_key_data, key, key_type_size);
            pos = 0;
        }
    }

    if (node->flag_is_leaf) {
        split = cell_insert(node, pos, record, key, key_type_size);
        if (errno)
            goto ERR;
        if (split == NULL)
            copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
        if (split) {
            res = (struct Split_res *)malloc(sizeof(struct Split_res));
            if (res == NULL) {
                errno = ENOMEM;
                goto ERR;
            }
            tmp_pointer = dalloc(disk); //new node
            node->last_pointer = tmp_pointer; //'last_pointer' of leaf node points to the next node
            copy_to_disk(split, disk->block_size, disk, tmp_pointer); //write new node to disk
            res->pointer = tmp_pointer;
            res->key = first_new_key(btree, node, split, key_type_size);
            free(split);
            if (disk_node != btree->root) {
                copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
            }
            else {
                tmp_pointer = dalloc(disk); //Allocs another disk space
                copy_to_disk(node, disk->block_size, disk, tmp_pointer);
                //set node as root
                node->flag_is_leaf = false;
                node->num = 1;
                node->pointers[0] = tmp_pointer;
                memcpy(node->key_data + key_type_size, res->key, key_type_size); //node->key_data remains the same.
                node->last_pointer = res->pointer;
                copy_to_disk(node, disk->block_size, disk, btree->root); //btree->root remains unchanged
                free(res);
                res = NULL;
            }
            goto END;
        }
    }

    else { //Not leaf node
        next = pos < node->num ? node->pointers[pos] : node->last_pointer;
        res = btree_insert_re(btree, next, key, record);
        if (errno)
            goto ERR;
        if (res) {
            split = cell_insert(node, pos + 1, res->pointer, res->key, key_type_size);
            if (errno)
                goto ERR;
            if (split == NULL) {
                copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
                free(res);
                res = NULL;
            }
            else {
                node->last_pointer = node->pointers[node->num - 1];
                tmp_pointer = dalloc(disk);
                res->pointer = tmp_pointer;
                res->key = first_nonempty_key(split, key_type_size);
                node->num--;
                
                copy_to_disk((void *)split, disk->block_size, disk, tmp_pointer); //write mew node to disk
                free(split);
                
                if (disk_node != btree->root) {
                    copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
                }
                else {
                    tmp_pointer = dalloc(disk); //Allocs another disk space
                    copy_to_disk(node, disk->block_size, disk, tmp_pointer);
                    //set node as root
                    node->flag_is_leaf = false;
                    node->num = 1;
                    node->pointers[0] = tmp_pointer;                    
                    memcpy(node->key_data + key_type_size, res->key, key_type_size); //node->key_data remains the same.
                    node->last_pointer = res->pointer; 
                    copy_to_disk(node, disk->block_size, disk, btree->root); //btree->root remains unchanged
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

vector_t *btree_select(PBTree btree, const void *key_start, const void *key_end) {
    if (btree == NULL)
        return NULL;
    DISK *disk = btree->disk;
    disk_pointer disk_node = btree->root;
    void *buffer = malloc(disk->block_size);
    if (buffer == NULL)
        return NULL;
    PNode node = NULL;
    void *p_key_data;
    size_t key_type_size = btree->p_key_type->get_type_size();
    vector_t *results = vector_create(0);
    if (results == NULL)
        return NULL;
    vector_set_type_size(results, sizeof(disk_pointer));
    int pos, i, compare_res;
    while (1) {
        copy_to_memory(disk, disk_node, buffer);
        node = (PNode)buffer;
        p_key_data = node->key_data;
        if (!node->flag_is_leaf) { //NOT leaf
            pos = -1;
            for (i = 0; i <= node->num; i++) {
                if (!(node->opt[i] & EMPTY_KEY)) {
                    compare_res = btree->p_key_type->compare(key_start, p_key_data + i * key_type_size);
                    if (compare_res > 0)
                        pos = i;
                    else {
                        if (compare_res == 0)
                            pos = i;
                        break;
                    }
                }
            } //for
            if (pos == -1)
               pos = 0; 
            disk_node = pos < node->num ? node->pointers[pos] : node->last_pointer;
            continue;
        }
        //Leaf node for the rest of while loop
        pos = -1;
        for (i = 0; i < node->num; i++) {
            if (btree->p_key_type->compare(key_start, p_key_data + i * key_type_size) <= 0) {
                pos = i; //first key larger than or equal to key_start
                break;
            }
        }
        //Looking for keys in interval [key_start, key_end]
        while (1) {
            for (; pos < node->num; pos++) {
                if (btree->p_key_type->compare(p_key_data + pos * key_type_size, key_end) <= 0)
                    vector_push(results, &node->pointers[pos]);
                else 
                    break;
            }
            if (pos < node->num)
                break;
            disk_node = node->last_pointer;
            if (disk_node == DNULL)
                break;
            buffer = copy_to_memory(disk, disk_node, buffer);
            node = (PNode) buffer;
            pos = 0;
        }
        break;
    } //while
    free(buffer);
    return results;
}
