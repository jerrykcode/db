#include "vector.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>

int VECTOR_SHALLOW_COPY = 0x00000001;
int VECTOR_REFERENCE_COPY = 0x00000002;

static const int VECTOR_INITIAL_CAPACITY = 10;

typedef struct Vector {
    int flags;
    size_t type_size;
    size_t vec_size;
    size_t vec_capacity;    
    void *arr;
    void (*destroy_element)(void *); //only used in case of VECTOR_SHALLOW_COPY
} *PVector;

static bool is_deep_copy(PVector vec) {
    if (vec->flags & VECTOR_SHALLOW_COPY)
        return false;
    if (vec->flags & VECTOR_REFERENCE_COPY)
        return false;
    return true;
}

vector_t *vector_create(int flags) {
    PVector vec = malloc(sizeof(struct Vector));
    if (vec == NULL)
        return NULL;
    vec->flags = flags;
    vec->vec_size = 0;
    vec->vec_capacity = VECTOR_INITIAL_CAPACITY;
    if (is_deep_copy(vec))
        vec->type_size = 0; //Initialize
    else {
        vec->type_size = sizeof(void *); //store pointers
        vec->arr = malloc(VECTOR_INITIAL_CAPACITY * vec->type_size);
        if (vec->arr == NULL) {
            free(vec);
            return NULL;
        }
    }
    vec->destroy_element = NULL;
    return vec;
}

int vector_set_type_size(vector_t *ptr, size_t type_size) {
    if (ptr == NULL || type_size == 0) //Invalid arguments
        return EINVAL;
    PVector vec = (PVector)ptr;
    if (!is_deep_copy(vec)) {
        return EINVAL;
    }
    else {
        if (vec->type_size) //vec->type_size != 0 => type size already set before
            return EINVAL;
        vec->type_size = type_size;
        vec->arr = malloc(VECTOR_INITIAL_CAPACITY * type_size);
        if (vec->arr == NULL)
            return ENOMEM;
    }
    return 0;
}

int vector_set_destroy_element_function(vector_t *ptr, void(*destroy_element)(void *)) {
    if (ptr == NULL)
        return EINVAL;
    PVector vec = (PVector) ptr;
    if (! vec->flags & VECTOR_SHALLOW_COPY)
        return EINVAL;
    if (vec->destroy_element != NULL)
        return EINVAL;
    vec->destroy_element = destroy_element;
    return 0;
}

void vector_destroy(vector_t *ptr) {
    if (ptr == NULL)
        return ;
    PVector vec = (PVector) ptr;
    if (vec->flags & VECTOR_SHALLOW_COPY && vec->destroy_element != NULL) {
        for (int i = 0; i < vec->vec_size; i++) {
            void *element = *(void **)(vec->arr + i * vec->type_size);
            if (element != NULL)
                vec->destroy_element(element);
        }            
    }
    if (vec->arr)
        free(vec->arr);
    free(vec);
}

size_t vector_size(vector_t *ptr) {
    if (ptr == NULL)
        return 0;
    PVector vec = (PVector) ptr;
    return vec->vec_size;
}

int vector_push(vector_t *ptr, void *element) {    
    if (ptr == NULL)
        return EINVAL;
    PVector vec = (PVector) ptr;    
    if (is_deep_copy(vec) && element == NULL)
        return EINVAL;
    if (vec->vec_size < vec->vec_capacity) {
        if (is_deep_copy(vec))
            memcpy(vec->arr + vec->vec_size * vec->type_size, element, vec->type_size);
        else
            memcpy(vec->arr + vec->vec_size * vec->type_size, (void *)&element, vec->type_size);
        vec->vec_size++;
    }
    else {
        void *new_arr = malloc(vec->vec_capacity * 2 * vec->type_size);
        if (new_arr == NULL)
            return ENOMEM;
        memcpy(new_arr, vec->arr, vec->vec_capacity * vec->type_size);
        free(vec->arr);
        vec->arr = new_arr;
        vec->vec_capacity *= 2;
        vector_push(vec, element);
    }
    return 0;
}

void *vector_get(vector_t *ptr, size_t index) {
    if (ptr == NULL)
        return NULL;
    PVector vec = (PVector) ptr;
    if (index >= vec->vec_size)
        return NULL;
    void *res = vec->arr + index * vec->type_size;
    if (is_deep_copy(vec))
        return res;
    else 
        return *(void **)res;
}
