#ifndef RBTREE_H__
#define RBTREE_H__

#include <stdbool.h>

typedef void rbtree_t;

/* flags */
extern int RBT_SHALLOW_COPY;
extern int RBT_REFERENCE_COPY;
extern int RBT_INSERT_REPLACE;

/* functions */
rbtree_t *rbtree_create(int (*compare)(const void *, const void *), void (*destroy)(void *), size_t key_size, int flags);
void rbtree_destroy(rbtree_t *);

int rbtree_insert(rbtree_t *, void *key);
void *rbtree_search(rbtree_t *, void *key);
int rbtree_remove(rbtree_t *, void *key);
size_t rbtree_size(rbtree_t *);

typedef void rbtree_iterator_t;

rbtree_iterator_t *rbtree_iterator_create(rbtree_t *);
void rbtree_iterator_destroy(rbtree_iterator_t *);
void *rbtree_iterator_current(rbtree_iterator_t *);
bool rbtree_iterator_has_next(rbtree_iterator_t *);
bool rbtree_iterator_next(rbtree_iterator_t *);

#endif
