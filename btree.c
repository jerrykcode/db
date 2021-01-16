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

static const uint8_t OPT_NONE  = 0x00;
static const uint8_t OPT_EMPTY_KEY = 0x01;
static const uint8_t OPT_INFINITY_KEY = 0x02;

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

//utility structure
struct Split_res {
    struct key_st *actual_key_pos;    
    struct key_st *this_node_key;
    struct key_st *new_node_key;
    disk_pointer new_node_pointer;
};
//utility structure
struct key_st {
    void *key_pointer;
    uint8_t key_opt;
};
static struct Split_res *btree_insert_re(PBTree btree, disk_pointer disk_node, struct key_st *key_pos, struct key_st *key_data, struct key_st *parent_key, record_t record);

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
    //insert an infinity key
    struct key_st *key_st = (struct key_st *)malloc(sizeof(struct key_st));
    if (key_st == NULL) {
        return NULL;
    }
    key_st->key_pointer = NULL;
    key_st->key_opt = OPT_INFINITY_KEY;
    btree_insert_re(btree, btree->root, key_st, key_st, NULL, DNULL);
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


/*
    B+ Tree manipulations
*/


static PNode cell_insert(PNode node, int pos, disk_pointer pointer, struct key_st *key, size_t key_type_size) {
    if (key == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (node->num < NODE_MAX_DEGREE) {
        if (pos <= node->num - 1) {
            //make room for pointer and key
            memmove(&node->pointers[pos + 1], &node->pointers[pos], (node->num - pos) * sizeof(disk_pointer));
            memmove(&node->opt[pos + 1], &node->opt[pos], (node->num - pos) * sizeof(uint8_t));
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
        node->opt[pos] = key->key_opt;
        if ( !(key->key_opt & OPT_EMPTY_KEY) && !(key->key_opt & OPT_INFINITY_KEY))
            memcpy(node->key_data + pos * key_type_size, key->key_pointer, key_type_size);
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
        memcpy(split->opt, &node->opt[split_start], (num - split_start + (node->flag_is_leaf ? 0 : 1)) * sizeof(uint8_t));
        memcpy(split->key_data, node->key_data + split_start * key_type_size, (num - split_start + (node->flag_is_leaf ? 0 : 1)) * key_type_size);
        //make room for key in node
        memmove(&node->pointers[pos + 1], &node->pointers[pos], (node->num - pos) * sizeof(disk_pointer));
        memmove(&node->opt[pos + 1], &node->opt[pos], (node->num - pos) * sizeof(uint8_t));
        memmove(node->key_data + (pos + 1) * key_type_size, node->key_data + pos * key_type_size, (node->num - pos) * key_type_size);
        //insert pointer and key into node
        node->pointers[pos] = pointer;
        node->opt[pos] = key->key_opt;
        if ( !(key->key_opt & OPT_EMPTY_KEY) && !(key->key_opt & OPT_INFINITY_KEY))
            memcpy(node->key_data + pos * key_type_size, key->key_pointer, key_type_size);
        node->num++;
    }
    else {
        if (pos > split_start) {
            //copy from [split_start, pos) of node to split
            memcpy(split->pointers, &node->pointers[split_start], (pos - split_start) * sizeof(disk_pointer));
            memcpy(split->opt, &node->opt[split_start], (pos - split_start) * sizeof(uint8_t));
            memcpy(split->key_data, node->key_data + split_start * key_type_size, (pos - split_start) * key_type_size);
        }
        //insert pointer and key into split
        if (node->flag_is_leaf || pos < num)
            split->pointers[pos - split_start] = pointer;
        else {
            split->pointers[pos - split_start] = split->last_pointer;
            split->last_pointer = pointer;
        }
        split->opt[pos - split_start] = key->key_opt;
        if ( !(key->key_opt & OPT_EMPTY_KEY) && !(key->key_opt & OPT_INFINITY_KEY))
            memcpy(split->key_data + (pos - split_start) * key_type_size, key, key_type_size);
        //copy the rest to split
        memcpy(&split->pointers[pos - split_start + 1], &node->pointers[pos], (num - pos) * sizeof(disk_pointer));
        /*
        Non-leaf node have one more key at the begin
        */
        memcpy(&split->opt[pos - split_start + 1], &node->opt[pos], (num - pos + (node->flag_is_leaf ? 0 : 1)) * sizeof(uint8_t));
        memcpy(split->key_data + (pos - split_start + 1) * key_type_size, node->key_data + pos * key_type_size, (num - pos + (node->flag_is_leaf ? 0 : 1)) * key_type_size);
        
        split->num++;
    }
    return split;
}

//insert recursively

static struct  Split_res *split_res_create() {
    struct Split_res *res = (struct Split_res *)malloc(sizeof(struct Split_res));
    if (res == NULL)
        return NULL;
    //Initialize
    res->actual_key_pos = NULL;
    res->this_node_key = NULL;
    res->new_node_key = NULL;
    res->new_node_pointer = DNULL;
    return res;
}

static void split_res_destroy(struct Split_res *res) {
    if (res == NULL)
        return;
    if (res->actual_key_pos)
        free(res->actual_key_pos);
    if (res->this_node_key)
        free(res->this_node_key);
    if (res->new_node_key)
        free(res->new_node_key);
    free(res);
}

//utility functions
static int first_new_key_index(PBTree btree, PNode node, PNode split, size_t key_type_size) {
    //look for the index of the first new key in 'split'('node' and 'split' are leaf nodes)
    for (int i = 0; i < split->num; i++)
        if (btree->p_key_type->compare(
            split->key_data + i * key_type_size,
            node->key_data + (node->num - 1) * key_type_size
            ) != 0)
        {
            return i; //index of first new key
        }
    return -1; //NO new key found
}

static int find_key_index(PBTree btree, PNode node, void *key, size_t key_type_size) {
    //'key' NOT NULL in this function
    //'node' is leaf node
    for (int i = 0; i < node->num; i++) {
        if (btree->p_key_type->compare(key, node->key_data + i * key_type_size) == 0)
            return i;
    }
    return -1;
}

static int first_nonempty_key_index(PNode node, size_t key_type_size) {
    //look for the index of the first non-empty node in 'node'('node' is non-leaf node)
    for (int i = 0; i <= node->num; i++) 
        if (! (node->opt[i] & OPT_EMPTY_KEY))
            return i;
    return -1;
}

static bool equal_key_st(const struct key_st *a, const struct key_st *b, int(*compare)(const void *, const void *)) {
    if (a == NULL || b == NULL)
        return false;

    {
        int a_is_inf = a->key_opt & OPT_INFINITY_KEY;
        int b_is_inf = b->key_opt & OPT_INFINITY_KEY;
        if (a_is_inf && b_is_inf)
            return true;
        if (a_is_inf || b_is_inf)
            return false;
    }
    // a is NOT infinity && b is NOT infinity

    {
        int a_is_empty = a->key_opt & OPT_EMPTY_KEY;
        int b_is_empty = b->key_opt & OPT_EMPTY_KEY;
        if (a_is_empty && b_is_empty)
            return true;
        if (a_is_empty || b_is_empty)
            return false;
    }
    // a is NOT empty && b is NOT empty

    return compare(a->key_pointer, b->key_pointer) == 0;
}

static int compare_key_st(const struct key_st *a, const struct key_st *b, int (*compare)(const void *, const void *)) {
    //Require the parameters to be
    //  a != NULL
    //  b != NULL
    //  a->key_pointer != NULL if a is not infinity_key
    //  b->key_pointer != NULL if b is not infinity_key
    int a_is_inf = a->key_opt & OPT_INFINITY_KEY;
    int b_is_inf = b->key_opt & OPT_INFINITY_KEY;
    if (a_is_inf && b_is_inf)
        return 0; //a equals b
    if (a_is_inf)
        return 1; //a > b
    if (b_is_inf)
        return -1; //a < b
    // !a_is_inf && !b_is_inf    
    return compare(a->key_pointer, b->key_pointer);
}


//insert recursively
static struct Split_res *btree_insert_re(PBTree btree, disk_pointer disk_node, struct key_st *key_pos, struct key_st *key_data, struct key_st *parent_key, record_t record) {
    DISK *disk = btree->disk;
    PNode node, split;
    void *p_key_data;
    size_t key_type_size = btree->p_key_type->get_type_size();
    int left, right, mid, compare_res;
    int pos, key_index, node_first_new_key_index;
    int i; //use by 'for loop'
    struct Split_res *res = NULL;
    struct key_st *key_to_compare, *tmp_parent_key;
    disk_pointer next, tmp_pointer;
    void *buffer, *tmp_buffer;
    int (*compare)(const void *, const void *);

    buffer = malloc(disk->block_size); //disk->block_size equals to get_node_size(btree->p_key_type->type_size())
    if (buffer == NULL) {
        errno = ENOMEM;
        goto ERR;
    }
    copy_to_memory(disk, disk_node, buffer); //read to memory
    node = (PNode)buffer;
    p_key_data = node->key_data;

    key_to_compare = (struct key_st *)malloc(sizeof(struct key_st));
    if (key_to_compare == NULL) {
        errno = ENOMEM;
        goto ERR;
    }

    compare = btree->p_key_type->compare;

    if (node->flag_is_leaf) {
        pos = 0;
        for (i = 0; i < node->num; i++) {
            key_to_compare->key_pointer = p_key_data + i * key_type_size;
            key_to_compare->key_opt = node->opt[i];
            compare_res = compare_key_st(key_pos, key_to_compare, compare);
            if (compare_res > 0)
                pos++;
            else //compare_res <= 0
                break;
        }
        if (pos == node->num && node->last_pointer != DNULL) {
            tmp_pointer = disk_node;
            disk_node = node->last_pointer;
            tmp_buffer = malloc(disk->block_size);
            if (tmp_buffer == NULL) {
                errno = ENOMEM;
                goto ERR;
            }
            copy_to_memory(disk, disk_node, tmp_buffer);
            node = (PNode) tmp_buffer;
            p_key_data = node->key_data;
            key_to_compare->key_pointer = p_key_data;
            key_to_compare->key_opt = node->opt[0];
            if (compare_key_st(key_pos, key_to_compare, compare) > 0) {
                pos = 1;
                while (1) {
                    for (i = pos; i < node->num; i++) {
                        key_to_compare->key_pointer = p_key_data + i * key_type_size;
                        key_to_compare->key_opt = node->opt[i];
                        compare_res = compare_key_st(key_pos, key_to_compare, compare);
                        if (compare_res > 0)
                            pos++;
                        else //compare_res <= 0
                            break;
                    }
                    if (pos < node->num)
                        break;
                    /*if (node->last_pointer == DNULL) ---------- This will never happen
                        break;*/
                    disk_node = node->last_pointer; 
                    copy_to_memory(disk, disk_node, buffer);
                    node = (PNode) buffer;
                    pos = 0;
                }
                free(tmp_buffer);
                res = split_res_create();
                if (res == NULL) {
                    errno = ENOMEM;
                    goto ERR;
                }
                res->actual_key_pos = (struct key_st *)malloc(sizeof(struct key_st));
                if (res->actual_key_pos == NULL) {
                    errno = ENOMEM;
                    goto ERR;
                }
                res->actual_key_pos->key_pointer = p_key_data + pos * key_type_size;
                res->actual_key_pos->key_opt = OPT_NONE;
                res->this_node_key = NULL;
                res->new_node_key = NULL;
                res->new_node_pointer = DNULL;
                goto END;
            }
            free(tmp_buffer);
            disk_node = tmp_pointer;
            node = (PNode) buffer;
            p_key_data = node->key_data;
        }
    }
    else {
        pos = -1;
        for (i = 0; i <= node->num; i++) {
            if (node->opt[i] & OPT_EMPTY_KEY)
                continue;
            key_to_compare->key_pointer = p_key_data + i * key_type_size;
            key_to_compare->key_opt = node->opt[i];
            compare_res = compare_key_st(key_pos, key_to_compare, compare);
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
            pos = 0;
        }
    }

    if (node->flag_is_leaf) {
        if (parent_key) {            
            if(compare_key_st(key_data, parent_key, compare) < 0) {
                node_first_new_key_index = pos;
                res = split_res_create();
                if (res == NULL) {
                    errno = ENOMEM;
                    goto ERR;
                }
                res->this_node_key = (struct key_st *)malloc(sizeof(struct key_st));
                if (res->this_node_key == NULL) {
                    errno = ENOMEM;
                    goto ERR;
                }
                res->this_node_key->key_pointer = key_data->key_pointer;
                res->this_node_key->key_opt = key_data->key_opt;
            }
            else {
                node_first_new_key_index = find_key_index(btree, node, parent_key->key_pointer, key_type_size);
            }
        }
        
        split = cell_insert(node, pos, record, key_data, key_type_size);
        if (errno)
            goto ERR;
        if (split == NULL)
            copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
        if (split) {
            if (res == NULL) {
                res = split_res_create();
                if (res == NULL) {
                    errno = ENOMEM;
                    goto ERR;
                }
            }
            tmp_pointer = dalloc(disk); //new node
            copy_to_disk(split, disk->block_size, disk, tmp_pointer); //write new node to disk
            node->last_pointer = tmp_pointer; //'last_pointer' of leaf node points to the next node
            res->new_node_pointer = tmp_pointer;
            res->new_node_key = (struct key_st *)malloc(sizeof(struct key_st));
            if (res->new_node_key == NULL) {
                errno = ENOMEM;
                goto ERR;
            }
            key_index = first_new_key_index(btree, node, split, key_type_size);
            if (key_index >= 0) {
                res->new_node_key->key_pointer = split->key_data + key_index * key_type_size;
                res->new_node_key->key_opt = split->opt[key_index];
            }
            else {
                res->new_node_key->key_pointer = NULL;
                res->new_node_key->key_opt = OPT_EMPTY_KEY;
            }
            free(split);
            if (node_first_new_key_index >= node->num) {
                if (res->this_node_key == NULL) {
                    res->this_node_key = (struct key_st *)malloc(sizeof(struct key_st));
                    if (res->this_node_key == NULL) {
                        errno = ENOMEM;
                        goto ERR;
                    }
                }
                res->this_node_key->key_pointer = NULL;
                res->this_node_key->key_opt = OPT_EMPTY_KEY;
            }
            
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
                memcpy(node->key_data + key_type_size, res->new_node_key->key_pointer, key_type_size); //node->key_data remains the same
                node->opt[1] = res->new_node_key->key_opt; //node->opt[0] remains the same
                node->last_pointer = res->new_node_pointer;
                copy_to_disk(node, disk->block_size, disk, btree->root); //btree->root remains unchanged
                free(res);
                res = NULL;
            }
            goto END;
        }
    }

    else { //Not leaf node
        next = pos < node->num ? node->pointers[pos] : node->last_pointer;
        tmp_parent_key = (struct key_st *)malloc(sizeof(struct key_st));
        if (tmp_parent_key == NULL) {
            errno = ENOMEM;
            goto ERR;
        }
        tmp_parent_key->key_pointer = p_key_data + pos * key_type_size;
        tmp_parent_key->key_opt = node->opt[pos];
        res = btree_insert_re(btree, next, key_pos, key_data, tmp_parent_key, record);
        free(tmp_parent_key);
        if (errno)
            goto ERR;
        if (res) {
            if (res->actual_key_pos != NULL) {
                //we should insert at another positiion
                goto END;
            }
            if (res->this_node_key) {
                //node->key[pos] changed
                node->opt[pos] = res->this_node_key->key_opt;
                if ( ! (node->opt[pos] & OPT_EMPTY_KEY) && !(node->opt[pos] & OPT_INFINITY_KEY)) {
                    memcpy(node->key_data + pos * key_type_size, res->this_node_key->key_pointer, key_type_size);
                }
            }
            /* 
            if (res->new_node_pointer == DNULL) {
                res->new_node_key = NULL;
                res->new_node_pointer = DNULL;
            } 
            */
            
            if (res->new_node_key) {
                split = cell_insert(node, pos + 1, res->new_node_pointer, res->new_node_key, key_type_size);
                if (errno)
                    goto ERR;
                if (split == NULL) {
                    copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
                    //will not split
                    free(res->new_node_key);
                    res->new_node_key = NULL;
                    res->new_node_pointer = DNULL; 
                }
                else {
                    node->last_pointer = node->pointers[node->num - 1];
                    tmp_pointer = dalloc(disk);
                    copy_to_disk((void *)split, disk->block_size, disk, tmp_pointer); //write mew node to disk
                    free(split);
                    res->new_node_pointer = tmp_pointer;
                    key_index = first_nonempty_key_index(split, key_type_size);
                    if (key_index >= 0) {
                        res->new_node_key->key_pointer = split->key_data + key_index * key_type_size;
                        res->new_node_key->key_opt = split->opt[key_index];
                    }
                    else {
                        res->new_node_key->key_pointer = NULL;
                        res->new_node_key->key_opt = OPT_EMPTY_KEY;
                    }
                    node->num--;
                } 
            } //if res->new_node_pointer != DNULL

            key_index = first_nonempty_key_index(node, key_type_size);
            if (key_index >= 0) {
                key_to_compare->key_pointer = node->key_data + key_index * key_type_size;
                key_to_compare->key_opt = node->opt[key_index];
            }
            else {
                key_to_compare->key_pointer = NULL;
                key_to_compare->key_opt = OPT_EMPTY_KEY;
            }
            
            if (equal_key_st(parent_key, key_to_compare, compare)) {
                if (res->this_node_key)
                    free(res->this_node_key);
                res->this_node_key = NULL;
            }
            else {
                res->this_node_key = (struct key_st *)malloc(sizeof(struct key_st));
                if (res->this_node_key == NULL) {
                    errno = ENOMEM;
                    goto ERR;
                }
                res->this_node_key->key_pointer = key_to_compare->key_pointer;
                res->this_node_key->key_opt = key_to_compare->key_opt;
            }

            if (res->new_node_key == NULL) {
                copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
                if (res->this_node_key == NULL) {
                    //No need 'res' anymore
                    split_res_destroy(res);
                    res = NULL;
                }
            }
            else if (disk_node != btree->root) {
                copy_to_disk(node, disk->block_size, disk, disk_node); //write back to disk
            }
            else {
                tmp_pointer = dalloc(disk); //Allocs another disk space
                copy_to_disk(node, disk->block_size, disk, tmp_pointer);
                //set node as root
                node->flag_is_leaf = false;
                node->num = 1;
                
                //node->key_data remains the same
                //node->opt[0] remains the same
                node->pointers[0] = tmp_pointer;
                
                node->opt[1] = res->new_node_key->key_opt;
                if (!(node->opt[1] & OPT_EMPTY_KEY) && !(node->opt[1] & OPT_INFINITY_KEY)){
                    memcpy(node->key_data + key_type_size, res->new_node_key->key_pointer, key_type_size);
                }
                node->last_pointer = res->new_node_pointer; 
                copy_to_disk(node, disk->block_size, disk, btree->root); //btree->root remains unchanged
                split_res_destroy(res);
                res = NULL;
            }            
        }
    }

END:
ERR:
    if (buffer)
        free(buffer);
    if (key_to_compare)
        free(key_to_compare);
    if (errno) {
        if (res) 
            split_res_destroy(res);
        return NULL;
    }

    return res;
}

int btree_insert(PBTree btree, void *key, record_t record) {
    if (btree == NULL || key == NULL) {
        errno = EINVAL;
        goto ERR;
    }

    errno = 0;
    struct key_st *key_st = (struct key_st *)malloc(sizeof(struct key_st));
    key_st->key_pointer = key;
    key_st->key_opt = OPT_NONE;
    struct Split_res *res = btree_insert_re(btree, btree->root, key_st, key_st, NULL, record);
    if (errno)
        goto ERR;
    if (res && res->actual_key_pos) {
        btree_insert_re(btree, btree->root, res->actual_key_pos, key_st, NULL, record);
        if (errno)
            goto ERR;
    }
    
ERR:
    return errno;
}

static vector_t *btree_select_static(PBTree btree, struct key_st *key_start, struct key_st *key_end) {
    if (btree == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    DISK *disk = btree->disk;
    disk_pointer disk_node = btree->root;
    void *buffer = malloc(disk->block_size);
    if (buffer == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    PNode node = NULL;
    void *p_key_data;
    size_t key_type_size = btree->p_key_type->get_type_size();
    vector_t *results = vector_create(0);
    if (results == NULL) {
        errno = ENOMEM;
        free(buffer);
        return NULL;
    }
    vector_set_type_size(results, sizeof(disk_pointer));
    int pos, i, compare_res;
    struct key_st *key_to_compare = (struct key_st *)malloc(sizeof(struct key_st));
    if (key_to_compare == NULL) {
        errno = ENOMEM;
        free(buffer);
        vector_destroy(results);
        return NULL;
    }
    int (*compare)(const void *, const void *) = btree->p_key_type->compare;
    while (1) {
        copy_to_memory(disk, disk_node, buffer);
        node = (PNode)buffer;
        p_key_data = node->key_data;
        if (!node->flag_is_leaf) { //NOT leaf
            pos = -1;
            for (i = 0; i <= node->num; i++) {
                if (!(node->opt[i] & OPT_EMPTY_KEY)) {
                    key_to_compare->key_pointer = p_key_data + i * key_type_size;
                    key_to_compare->key_opt = node->opt[i];
                    compare_res = compare_key_st(key_start, key_to_compare, compare);
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
            key_to_compare->key_pointer = p_key_data + i * key_type_size;
            key_to_compare->key_opt = node->opt[i];
            if (compare_key_st(key_start, key_to_compare, compare) <= 0) {
                pos = i; //first key larger than or equal to key_start
                break;
            }
        }
        //Looking for keys in interval [key_start, key_end]
        while (1) {
            for (; pos < node->num; pos++) {
                key_to_compare->key_pointer = p_key_data + pos * key_type_size;
                key_to_compare->key_opt = node->opt[i];
                if (compare_key_st(key_to_compare, key_end, compare) <= 0)
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
    free(key_to_compare);
    return results;
}

vector_t *btree_select(PBTree btree, const void *key_start, const void *key_end) {
    if (key_start == NULL || key_end == NULL) {
        errno = EINVAL;
        return NULL;
    }
    struct key_st *key_st_start = (struct key_st *)malloc(sizeof(struct key_st));
    if (key_st_start == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    key_st_start->key_pointer = key_start;
    key_st_start->key_opt = OPT_NONE;
    struct key_st *key_st_end = (struct key_st *)malloc(sizeof(struct key_st));
    if (key_st_end == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    key_st_end->key_pointer = key_end;
    key_st_end->key_opt = OPT_NONE;
    vector_t *results = btree_select_static(btree, key_st_start, key_st_end);
    free(key_st_start);
    free(key_st_end);
    return results;
}
