#ifndef STACK_H__
#define STACK_H__

typedef void stack_t;

stack_t *stack_create(size_t data_size, int flags);

int STACK_SHALLOW_COPY;
int STACK_REFERENCE_COPY;

void stack_destroy(stack_t *);

int stack_push(stack_t *, void *p_data);
void *stack_top(stack_t *);
int stack_pop(stack_t *);
size_t stack_size(stack_t *);

#endif
