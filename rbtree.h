#ifndef RBTREE_H__
#define RBTREE_H__

#include <stdbool.h>

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

/* flags */
extern int RBT_DEEP_COPY;
extern int RBT_INSERT_REPLACE;

/* functions */
PRBTree rbtree_create(int (*compare)(const void *, const void *), size_t key_size, int flags);
void rbtree_destroy(PRBTree p_rbtree);

int rbtree_insert(PRBTree p_rbtree, void *key);
void *rbtree_search(PRBTree p_rbtree, void *key);
int rbtree_remove(PRBTree p_rbtree, void *key);
size_t rbtree_size(PRBTree p_rbtree);

typedef void rbtree_iterator_t;

rbtree_iterator_t *rbtree_iterator_create(PRBTree p_rbtree);
void rbtree_iterator_destroy(rbtree_iterator_t *);
void *rbtree_iterator_current(rbtree_iterator_t *);
bool rbtree_iterator_has_next(rbtree_iterator_t *);
bool rbtree_iterator_next(rbtree_iterator_t *);

#endif
