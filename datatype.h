#ifndef DATATYPE_H__
#define DATATYPE_H__
#include <stdlib.h>

//The data types will later be replaced by enum
size_t type_size(const char *type_name);

int is_valid_datatype(const char *type_name);

typedef struct {
    //virtual functions
    const char *(*get_type_name)(void);
    size_t (*get_type_size)(void);
    void (*cpy_to_memory)(void *, const char *);
} DataType;

DataType *int_data_type();
void free_int_data_type();

DataType *bigint_data_type();
void free_bigint_data_type();

DataType *get_data_type(const char *type_name);

#endif
