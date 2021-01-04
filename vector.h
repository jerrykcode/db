#ifndef VECTOR_H__
#define VECTOR_H__

#include <stdlib.h>

typedef void vector_t;

extern int VECTOR_SHALLOW_COPY;
extern int VECTOR_REFERENCE_COPY;

vector_t *vector_create(int flags);
int vector_set_type_size(vector_t *, size_t type_size); //Only used in case of deep copy, returns 0 if success
int vector_set_destroy_element_function(vector_t *, void(*destroy_element)(void *)); //Only used in case of VECTOR_SHALLOW_COPY, returns 0 if success
void vector_destroy(vector_t *);

size_t vector_size(vector_t *);

int vector_push(vector_t *, void *element); //Returns 0 if success
void *vector_get(vector_t *, size_t index);

#endif
