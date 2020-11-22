#include "datatype.h"
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

/*

  Data type for int (32 bit signed integer)    

*/
static const char *int_get_type_name(void) {
    return "int";
}

static size_t int_get_type_size(void) {
    return sizeof(int);
}

static int int_compare(const void *a, const void *b) {
    return *((int *)a) - *((int *)b);
}

static void *int_convert_to_val(const char *intstr) {
    errno = 0;
    char *endptr;
    long val = strtol(intstr, &endptr, 10);
    
    //checking for errors
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
            || (errno != 0 && val == 0)) {
        perror("strol()");
        return NULL;
    }
    
    if (sizeof(long) == 8 && (val > INT_MAX || val < INT_MIN)) {
        errno = ERANGE;        
        return NULL;
    }
    
    if (endptr == intstr) {
        fprintf(stderr, "Error in strtol(): no digits were found\n");
        return NULL;
    }

    //If we got here, strtol() successfully parsed a number
    int *p_val = (int *)malloc(sizeof(int));
    *p_val = (int)val;
    return (void *)p_val;
}

static void int_cpy_to_memory(void *memory, const char *intstr) {
    void *p_val = int_convert_to_val(intstr);
    if (p_val != NULL)
        memcpy(memory, p_val, sizeof(int));
    free(p_val);
}

static void int_print(const void *memory) {
    void *m = malloc(sizeof(int));
    memcpy(m, memory, sizeof(int));
    printf("%d", *(int *)m);
    free(m);
}

//constructor
static DataType *new_int_data_type() {
    DataType *type = (DataType *)malloc(sizeof(DataType));
    type->get_type_name  = int_get_type_name;
    type->get_type_size  = int_get_type_size;
    type->compare        = int_compare;
    type->convert_to_val = int_convert_to_val;
    type->cpy_to_memory  = int_cpy_to_memory;
    type->print          = int_print;
    return type;
}
static DataType *int_data_type_; //singleton

DataType *int_data_type() {
    //only one thread at this time
    if (int_data_type_ == NULL) {
        int_data_type_ = new_int_data_type();
    }
    return int_data_type_;
}

void free_int_data_type() {
    if (int_data_type_ == NULL)
        return;
    free(int_data_type_);
    int_data_type_ = NULL;
}

/*

  Data type for bigint (64 bit signed integer)

*/
static const char *bigint_get_type_name(void) {
    return "bigint";
}

static size_t bigint_get_type_size(void) {
    return sizeof(long long);
}

static int bigint_compare(const void *a, const void *b) {
    return *((long long *)a) - *((long long *)b);
}

static void *bigint_convert_to_val(const char *bigintstr) {
    errno = 0;
    char *endptr;
    long long val = strtoll(bigintstr, &endptr, 10);
    
    if ((errno == ERANGE && (val == LLONG_MAX || val == LLONG_MIN))
           || (errno != 0 && val == 0)) {
        perror("strtoll()");
        return NULL;
    }
    if (endptr == bigintstr) {
        fprintf(stderr, "Error in strtoll(): no digits were found\n");
        return NULL;
    }

    long long *p_val = (long long *)malloc(sizeof(long long));
    *p_val = val;
    return (void *)p_val;
}

static void bigint_cpy_to_memory(void *memory, const char *bigintstr) {
    void *p_val = bigint_convert_to_val(bigintstr);
    if (p_val != NULL)
        memcpy(memory, p_val, sizeof(long long));
    free(p_val);
}

static void bigint_print(const void *memory) {
    void *m = malloc(sizeof(long long));
    memcpy(m, memory, sizeof(long long));
    printf("%lld", *(long long *)m);
    free(m);
}

//constructor
static DataType *new_bigint_data_type() {
    DataType *type = (DataType *)malloc(sizeof(DataType));
    type->get_type_name  = bigint_get_type_name;
    type->get_type_size  = bigint_get_type_size;
    type->compare        = bigint_compare;
    type->convert_to_val = bigint_convert_to_val;
    type->cpy_to_memory  = bigint_cpy_to_memory;
    type->print          = bigint_print;
    return type;
}
static DataType *bigint_data_type_; //singleton

DataType *bigint_data_type() {
    //NO multi-thread at this time
    if (bigint_data_type_ == NULL) {
        bigint_data_type_ = new_bigint_data_type();
    }
    return bigint_data_type_;
}

void free_bigint_data_type() {
    if (bigint_data_type_ == NULL)
        return;
    free(bigint_data_type_);
    bigint_data_type_ = NULL;
}

/*

*/

DataType *get_data_type(const char *type_name) {
    if (strcmp(int_data_type()->get_type_name(), type_name) == 0)
        return int_data_type();
    else if (strcmp(bigint_data_type()->get_type_name(), type_name) == 0)
        return bigint_data_type();
    return NULL;
}

size_t type_size(const char *type_name) {
    return get_data_type(type_name)->get_type_size();
}

int is_valid_datatype(const char *type_name) {
    return (get_data_type(type_name) != NULL);
}
