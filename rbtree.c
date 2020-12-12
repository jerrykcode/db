#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rbtree.h"
#include "stack.h"

int RBT_SHALLOW_COPY = 0x00000001;
int RBT_REFERENCE_COPY = 0x00000002;
int RBT_INSERT_REPLACE = 0x00000004;

typedef enum {
    RED,
    BLACK
} Color;

typedef struct TNode {
    void *key;
    Color color;
    struct TNode *parent, *left, *right;
} *PTNode;

typedef struct RBTree {
    PTNode root;
    int (*compare)(const void *, const void *);
	size_t key_size;
    int flags;
    size_t num_nodes;
} *PRBTree;

rbtree_t *rbtree_create(int (*compare)(const void *, const void *), size_t key_size, int flags) {
    PRBTree rbtree = (PRBTree)malloc(sizeof(struct RBTree));
    if (rbtree == NULL) {
        return NULL;
    }
    rbtree->root = NULL;
    rbtree->compare = compare;
	rbtree->key_size = key_size;
    rbtree->flags = flags;
    rbtree->num_nodes = 0;
    return rbtree;
}

static void delete_node(PTNode node) {
    if (node == NULL)
        return;
    delete_node(node->left);
    delete_node(node->right);
    free(node);
}

void rbtree_destroy(rbtree_t *ptr) {
    if (ptr == NULL) 
        return;
    PRBTree rbtree = (PRBTree)ptr;
    delete_node(rbtree->root);
    free(rbtree);
}

static PTNode get_brother(PTNode node) {
    if (node == NULL)
        return NULL;
    PTNode parent = node->parent;
    if (parent == NULL)
        return NULL;
    if (parent->left == node)
        return parent->right;
    else
        return parent->left;
}

static void left_rotate(PRBTree rbtree, PTNode node) {
    PTNode parent = node->parent;
    bool flag_left = parent && parent->left == node;
    PTNode k = node->right;
    node->right = k->left;
    k->left->parent = node;
    k->left = node;
    node->parent = k;
    k->parent = parent;
    if (parent) {
        if (flag_left)
            parent->left = k;
        else
            parent->right = k;
    }
    else
        rbtree->root = k;
}

static void right_rotate(PRBTree rbtree, PTNode node) {
    PTNode parent = node->parent;
    bool flag_left = parent && parent->left == node;
    PTNode k = node->left;
    node->left = k->right;
    k->right->parent = node;
    k->right = node;
    node->parent = k;
    k->parent = parent;
    if (parent) {
        if (flag_left)
            parent->left = k;
        else
            parent->right = k;
    }
    else
        rbtree->root = k;
}

static void rbtree_insert_fix(PRBTree rbtree, PTNode node) {
    while (1) {
        PTNode parent = node->parent;
        if (parent == NULL) {
            node->color = BLACK;
            return;
        }
        if (parent->color == BLACK)
            return;
        PTNode grand = parent->parent;
        PTNode uncle = get_brother(parent);
        if (uncle && uncle->color == RED) {
            parent->color = BLACK;
            uncle->color = BLACK;
            grand->color = RED;
            node = grand;
            continue;
        }
        if (node == parent->right && parent == grand->left) {
            /*
                case1:
                         grand(black)
                         /          \
                        /            \
                    parent(red)     uncle(NULL or black) 
                            \
                             \
                            node(red)
            */
            left_rotate(rbtree, parent);
            node = parent;
            //move to case 2
            continue;
        }    
        if (node == parent->left && parent == grand->left) {
            /*
                case2:
                          grand(black)                                  
                         /          \
                        /            \
                    parent(red)     uncle(NULL or black)
                    /
                   /
                node(red)                       

                fixed:
                         parent(black)
                         /          \
                        /            \
                    node(red)     grand(red)
                                        \
                                         \
                                        uncle(NULL or black)

            */
            parent->color = BLACK;
            grand->color  = RED;
            right_rotate(rbtree, grand);
            //fixed
            break;
        }
        //symmetric cases
        if (node == parent->left && parent == grand->right) {
            right_rotate(rbtree, parent);
            node = parent;
            continue;
        }
        if (node == parent->right && parent == grand->right) {
            parent->color = BLACK;
            grand->color = RED;
            left_rotate(rbtree, grand);
            break;
        }
    }
}

int rbtree_insert(rbtree_t *ptr, void *key) {
    if (ptr == NULL)
        return EINVAL;
    PRBTree rbtree = (PRBTree)ptr;
    if (key == NULL && (! rbtree->flags & RBT_REFERENCE_COPY))
        return EINVAL;
    PTNode node = rbtree->root;
    PTNode parent = NULL;
    bool flag_left = false;
    while (node) {
        int compare_res = rbtree->compare(key, node->key);
        if (compare_res == 0) {
            //key equals to node->key
            if (rbtree->flags & RBT_INSERT_REPLACE) {
                if (rbtree->flags & RBT_SHALLOW_COPY || rbtree->flags & RBT_REFERENCE_COPY) {
                    void *old = node->key;
                    node->key = key;
                    if ( ! rbtree->flags & RBT_REFERENCE_COPY)
                        free(old);
                }                
                else {
                    memcpy(node->key, key, rbtree->key_size);
                }
            }
            return 0;
        }
        else if (compare_res < 0) {
            //key < node->key
            parent = node;
            node = node->left;
            flag_left = true;
        }
        else {
            //key > node->key
            parent = node;
            node = node->right;
            flag_left = false;
        }
    }

    node = (PTNode)malloc(sizeof(struct TNode));
    if (node == NULL) {
        return ENOMEM;
    }
    node->key = NULL;
    rbtree->num_nodes++;    
    if (rbtree->flags & RBT_SHALLOW_COPY || rbtree->flags & RBT_REFERENCE_COPY) {
        node->key = key;
    }
    else {
        node->key = malloc(rbtree->key_size);
        if (node->key == NULL) {
            free(node);
            return ENOMEM;
        }
        memcpy(node->key, key, rbtree->key_size); 
    }
    node->color = RED;
    node->parent = parent;
    node->left = node->right = NULL;
    if (parent) {
        if (flag_left)
            parent->left = node;
        else
            parent->right = node;
    }
    else
        rbtree->root = node;

    rbtree_insert_fix(rbtree, node);
    return 0;
}

void *rbtree_search(rbtree_t *ptr, void *key) {
    if (ptr == NULL)
        return NULL;
    PRBTree rbtree = (PRBTree)ptr;
    if (key == NULL && (! rbtree->flags & RBT_REFERENCE_COPY))
        return NULL;
    PTNode node = rbtree->root;
    while (node) {
        int compare_res = rbtree->compare(key, node->key);
        if (compare_res == 0) {
            return node->key;
        }
        if (compare_res < 0)
            node = node->left;
        else
            node = node->right;
    }
    return NULL;
}

static void rbtree_remove_fix(PRBTree rbtree, PTNode node) {
    while (1) {
        if (node == NULL)
            return;
        if (node->color == RED) {
            node->color = BLACK;
            return;
        }
        PTNode parent = node->parent;
        if (parent == NULL)
            return;
        PTNode brother = get_brother(node);
        //node is black => brother is not null
        if (brother->color == RED) {
            //brother is red => parent is black
            if (node == parent->left) {
                /*
                    case1:

                                parent(black)
                                /           \
                               /             \
                        node(black)         brother(red)
                                            /       \
                                           /         \
                                NULL or black       NULL or black

                    We change this case to:

                                    brother(black)
                                    /           \
                                   /             \
                            parent(red)         NULL or black
                            /       \
                           /         \
                    node(black)     NULL or black
                    
                */
                brother->color = BLACK;
                parent->color  = RED;
                left_rotate(rbtree, parent);
                continue;
            }
            else { //node == parent->right
                /* case2 : symmetric to case 1 */
                brother->color = BLACK;
                parent->color  = RED;
                right_rotate(rbtree, parent);
                continue;
            }
        }
        else { //brother->color == BLACK
            if (!(brother->left && brother->left->color == RED) &&
                !(brother->right && brother->right->color == RED)) {
                //children of brother are NOT red
                brother->color = RED;
                node = parent;
                continue;
            }
            else { //One of the children of brother is red
                if (node == parent->left) {
                    if(brother->right == NULL || brother->right->color != RED) {
                        /*
                            case a: 
                                        parent(red or black)
                                        /                   \
                                       /                     \
                                node(black)             brother(black)
                                                        /           \
                                                       /             \
                                                    (red)       (NULL or black)

                            We change this case to(case b):

                                        parent(red or black)
                                        /                   \
                                       /                     \
                                node(black)                 (black)
                                                                 \
                                                                  \
                                                                brother(red)
                                                                        \
                                                                         \
                                                                    (NULL or black)
                        */

                        brother->color = RED;
                        brother->left->color = BLACK;
                        right_rotate(rbtree, brother);
                        continue;
                    }
                    else { //brother->right && brother->right->color == RED
                        /*
                            case b:
                                         parent(red or black)
                                        /                   \
                                       /                     \
                                node(black)             brother(black)
                                                                    \
                                                                     \
                                                                    (red)
                            We change this case to(fixed):
                        
                                        brother(red or black)
                                        /                   \
                                       /                     \
                                parent(black)               (black)
                                /
                               /
                           node(black)
                        */

                        Color tmp = parent->color;
                        parent->color = brother->color;
                        brother->color = tmp;

                        left_rotate(rbtree, parent);
                        brother->right->color = BLACK;
                        break;
                    }
                }
                else { //node == parent->right
                    if (brother->left == NULL || brother->left->color != RED) {
                        /* symmetric to case a */
                        brother->color = RED;
                        brother->right->color = BLACK;
                        left_rotate(rbtree, brother);
                        continue;
                    }
                    else { //brother->left && brother->left->color == RED
                        /* symmetric to case b */
                        Color tmp = parent->color;
                        parent->color = brother->color;
                        brother->color = tmp;
                        
                        right_rotate(rbtree, parent);
                        brother->left->color = BLACK;
                        break;
                    }
                }
            }
        }
    }
}

int rbtree_remove(rbtree_t *ptr, void *key) {
    if (ptr == NULL)
        return EINVAL;
    PRBTree rbtree = (PRBTree)ptr;
    if (key == NULL && (! rbtree->flags & RBT_REFERENCE_COPY))
        return EINVAL;
    PTNode node = rbtree->root;
    while (node) {
        int compare_res = rbtree->compare(key, node->key);
        if (compare_res < 0) {
            //key < node->key
            node = node->left;
        }
        else if (compare_res > 0) {
            //key > node->key
            node = node->right;
        }
        else { //compare_res == 0
            rbtree->num_nodes--;

            if (node->left && node->right) {
                PTNode left_max = node->left;
                while (left_max->right)
                    left_max = left_max->right;
                //replace key of node by left_max
                memcpy(node->key, left_max->key, rbtree->key_size);
                node = left_max;
            }
            PTNode parent = node->parent;
            PTNode child = node->left ? node->left : node->right;
            Color rm_color = node->color;
            bool flag_left = parent && parent->left == node;

            free(node->key);
            free(node);
            if (child)
                child->parent = parent;

            if (parent) {
                if (flag_left)
                    parent->left = child;
                else
                    parent->right = child;
            }
            else
                rbtree->root = child;

            if (rm_color == BLACK)
                rbtree_remove_fix(rbtree, child);

            break;
        }        
    }
    return 0;
}

size_t rbtree_size(rbtree_t *ptr) {
    if (ptr == NULL)
        return 0;
    return ((PRBTree)ptr)->num_nodes;
}

/*
    Iterator 
*/
typedef struct Iterator {
    PTNode current;
    PTNode next;
    stack_t *stack;
} *PIterator;

rbtree_iterator_t *rbtree_iterator_create(rbtree_t *ptr) {
    if (ptr == NULL)
        return NULL;
    PRBTree rbtree = (PRBTree)ptr;
    PIterator iterator = malloc(sizeof(struct Iterator));
    if (iterator == NULL)
        goto END;
    PTNode node = rbtree->root;
    if (node == NULL) {
        iterator->current = NULL;
        iterator->next = NULL;
        goto END;
    }
    while (node->left) {
        node = node->left;
    }
    iterator->current = node;
    iterator->next = node;
    iterator->stack = stack_create(0, STACK_REFERENCE_COPY);
END:
    return iterator;
}

void rbtree_iterator_destroy(rbtree_iterator_t *ptr) {
    if (ptr == NULL)
        return;
    PIterator iterator = (PIterator)ptr;
    stack_destroy(iterator->stack);
    free(iterator);
}

void *rbtree_iterator_current(rbtree_iterator_t *ptr) {
    if (ptr == NULL)
        return NULL;
    return ((PIterator)ptr)->current->key;
}

bool rbtree_iterator_has_next(rbtree_iterator_t *ptr) {
    if (ptr == NULL)
        return false;
    PIterator iterator = (PIterator)ptr;    
    PTNode current = iterator->current;
    PTNode next = iterator->next;
    if (current != next)
        return true;
    if (current == NULL)
        return false;
    
    stack_t *stack = iterator->stack;
    if (current->right) {
        stack_push(stack, current);
        next = current->right;
        while (next->left)
            next = next->left;
        iterator->next = next;
        return true;
    }

    if (current->parent) {
        PTNode node = current->parent;
        while (node && stack_top(stack) == node) {
            node = node->parent;
            stack_pop(stack);
        }
        if (node) {
            iterator->next = node;
            return true;
        }
        else
            return false;
    }

    return false;
}

bool rbtree_iterator_next(rbtree_iterator_t *ptr) {
    if (ptr == NULL)
        return false;
    PIterator iterator = (PIterator)ptr;
    if (iterator->next != iterator->current) {
        iterator->current = iterator->next;
        return true;
    }
    else {
        if (rbtree_iterator_has_next(ptr))
            return rbtree_iterator_next(ptr);
        else
            return false;
    }
}
