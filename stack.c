#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "stack.h"

int STACK_SHALLOW_COPY = 0x00000001;
int STACK_REFERENCE_COPY = 0x00000002;

typedef struct Node {
    void *data;
    struct Node *pre;
} *PNode;

typedef struct Stack {
    PNode top;
    size_t data_size;
    int flags;
    size_t size;
} *PStack;

stack_t *stack_create(size_t data_size, int flags) {
    struct Stack *stack = malloc(sizeof(struct Stack));
    if (stack == NULL)
        return NULL;
    stack->data_size = data_size;
    stack->flags = flags;
    stack->top = NULL;
    stack->size = 0;
    return stack;
}

void stack_destroy(stack_t *ptr) {
    if (ptr == NULL)
        return;
    PStack stack = (PStack)ptr;
    PNode node = stack->top;
    while (node != NULL) {
        PNode tmp = node;
        node = node->pre;
        if (stack->flags & STACK_REFERENCE_COPY)
            free(tmp->data);
        free(tmp);
    }
    free(stack);
}

int stack_push(stack_t *ptr, void *p_data) {
    if (ptr == NULL || p_data == NULL)
        return EINVAL;
    PStack stack = (PStack)ptr;
    PNode node = malloc(sizeof(struct Node));
    node->data = NULL;
    if (node == NULL)
        return ENOMEM;
    if (stack->flags & STACK_SHALLOW_COPY || stack->flags & STACK_REFERENCE_COPY) {
        node->data = p_data;
    }
    else {
        node->data = malloc(sizeof(*p_data));
        if (node->data == NULL) {
            free(node);
            return ENOMEM;
        }
        memcpy(node->data, p_data, stack->data_size);
    }
    node->pre = stack->top;
    stack->top = node;
    stack->size++;
    return 0;
}

void *stack_top(stack_t *ptr) {
    if (ptr == NULL)
        return NULL;
    PStack stack = (PStack)ptr;
    return stack->top->data;
}

int stack_pop(stack_t *ptr) {
    if (ptr == NULL)
        return EINVAL;
    PStack stack = (PStack)ptr;
    PNode node = stack->top;
    if (node == NULL)
        return ENOTSUP;
    stack->top = node->pre;
    if (stack->flags & STACK_REFERENCE_COPY)
        free(node->data);
    free(node);
    stack->size--;
    return 0;
}

size_t stack_size(stack_t *ptr) {
    if (ptr == NULL)
        return EINVAL;
    return ((PStack)ptr)->size;
}
