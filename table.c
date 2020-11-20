#include "table.h"
#include "datatype.h"
#include "frame.h"
#include <error.h>

static char *get_data_pathname(const char *path, const char *table_name) {
    size_t path_len = strlen(path);
    size_t table_name_len = strlen(table_name);
    size_t data_suffix_len = strlen(DATA_SUFFIX);
    char *data_pathname = (char *)malloc(path_len + table_name_len + data_suffix_len);
    strcpy(data_pathname, path);
    strcpy(data_pathname + path_len, table_name);
    strcpy(data_pathname + path_len + table_name_len, DATA_SUFFIX);
    return data_pathname;
}

static char *get_frm_pathname(const char *path, const char *table_name) {
    size_t path_len = strlen(path);
    size_t table_name_len = strlen(table_name);
    size_t frm_suffix_len = strlen(FRAME_SUFFIX);
    char *frm_pathname = (char *)malloc(path_len + table_name_len + frm_suffix_len);
    strcpy(frm_pathname, path);
    strcpy(frm_pathname + path_len, table_name);
    strcpy(frm_pathname + path_len + table_name_len, FRAME_SUFFIX);
    return frm_pathname;
}

static void *cpy_to_buffer(const char *table_name, KeyList *list, KeyTypeMap *map, size_t *p_buffersize, size_t *p_blocksize) {
    size_t table_name_size = strlen(table_name);
    if (table_name_size > FRM_TABLE_NAME_SIZE) {
        fprintf(stderr, "Table name:\'%s\' too long", table_name);
        return NULL;
    }
    size_t num_keys = list_size(list);
    *p_buffersize = FRM_SIZE(num_keys);
    void *buffer = malloc(*p_buffersize);
    memcpy(buffer + FRM_TABLE_NAME_OFFSET, (void *)table_name, table_name_size);
    memcpy(buffer + FRM_NUM_KEYS_OFFSET, (void *)(&num_keys), FRM_NUM_KEYS_SIZE);
    *p_blocksize = 0;
    for (int i = 0; i < num_keys; i++) {
        char *name = list_get(list, i);
        char *type = map_get(map, name);
        size_t name_size = strlen(name);
        if (name_size > FRM_KEY_NAME_SIZE || !is_valid_datatype(type)) {
            if (name_size > FRM_KEY_NAME_SIZE)
                fprintf(stderr, "Key name: \'%s\' too long!\n", name);
            else
                fprintf(stderr, "Key: \'%s\' with invalid data type: \'%s\'\n", name, type);
            free(buffer);
            return NULL;
        }
        memcpy(buffer + FRM_KEY_NAME_OFFSET(i), name, name_size);
        memcpy(buffer + FRM_KEY_TYPE_OFFSET(i), type, strlen(type));
        *p_blocksize += type_size(type);
    }
    return buffer;
}

Table *table_create(const char *path, const char *table_name, KeyList *list, KeyTypeMap *map) {

    size_t buffer_size, block_size;
    void *buffer = cpy_to_buffer(table_name, list, map, &buffer_size, &block_size);
    if (buffer == NULL) {
        return NULL;
    }

    char *data_pathname = get_data_pathname(path, table_name);

    //create disk for data
   	DISK *data = dcreate(data_pathname, block_size);
    free(data_pathname);
    if (data == NULL) {
        fprintf(stderr, "error in dcreate()!");
        free(buffer);
        return NULL;
    }

    //write table attributes to the frame file
    char *frm_pathname = get_frm_pathname(path, table_name);
    FILE *frm = fopen(frm_pathname, "w+");
    free(frm_pathname);
    if (frm == NULL) {
        perror("fopen()");
        free(buffer);
        dclose(data);
        return NULL;
    }
    fwrite(buffer, buffer_size, 1, frm);
    fclose(frm);
    free(buffer);

    //malloc table
    Table *table = (Table *)malloc(sizeof(Table));
    table->map = map;
    table->list = list;
    table->data = data;

    return table;
}

Table *table_open(const char *path, const char *table_name) {
    char *frm_pathname = get_frm_pathname(path, table_name);
    FILE *frm = fopen(frm_pathname, "r+");
    free(frm_pathname);
    if (frm == NULL) {
        perror("fopen()");
        return NULL;
    }
    fseek(frm, FRM_NUM_KEYS_OFFSET, SEEK_SET);
    size_t num_keys;
    fread(&num_keys, sizeof(size_t), 1, frm);
    size_t buffer_size = FRM_SIZE(num_keys);
    void *buffer = malloc(buffer_size);
    fseek(frm, 0, SEEK_SET);
    fread(buffer, buffer_size, 1, frm);
    fclose(frm);
    KeyList *list = new_list();
    KeyTypeMap *map = new_map();
    size_t block_size;
    for (int i = 0; i < num_keys; i++) {
        char *name = (char *)malloc(FRM_KEY_NAME_SIZE);
        memcpy(name, buffer + FRM_KEY_NAME_OFFSET(i), FRM_KEY_NAME_SIZE);
        list_add(list, name);
        char *type = (char *)malloc(FRM_KEY_TYPE_SIZE);
        memcpy(type, buffer + FRM_KEY_TYPE_OFFSET(i), FRM_KEY_TYPE_SIZE);
        map_put(map, name, type);
    } 
    free(buffer);
    char *data_pathname = get_data_pathname(path, table_name);
    DISK *data = dopen(data_pathname);
    free(data_pathname);
    if (data == NULL) {
        fprintf(stderr, "error in dopen()!\n");
        for (int i = 0; i < num_keys; i++)
            free(list_get(list, i));
        list_free(list);
        char **types = (char **)malloc(num_keys * sizeof(char *));
        map_get_all_values(map, types);
        for (int i = 0; i < num_keys; i++)
            free(types[i]);
        free(types);
        map_free(map);
        return NULL;
    }
    Table *table = (Table *)malloc(sizeof(Table));
    table->map = map;
    table->list = list;
    table->data = data;
    return table;
}

void table_close(Table *table) {
    List *list = table->list;
    size_t num_keys = list_size(list);
    Map *map = table->map;
    for (int i = 0; i < num_keys; i++)
        free(list_get(list, i));
    list_free(list);
    char **types = (char **)malloc(num_keys * sizeof(char *));
    map_get_all_values(map, types);
    for (int i = 0; i < num_keys; i++)
        free(types[i]);
    free(types);
    map_free(map);
    dclose(table->data);
    free(table);
}

void insert(Table *table, KeyValueMap *map) {

}


