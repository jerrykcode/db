#include "table.h"
#include "datatype.h"
#include "frame.h"
#include <errno.h>

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

void table_insert(Table *table, KeyValueMap *map) {
    size_t block_size = table->data->block_size;
    void *memory = malloc(block_size);
    KeyList *list = table->list;
    KeyTypeMap *kt_map = table->map;
    size_t offset = 0;
    for (int i = 0; i < list_size(list); i++) {
        char *key_name = list_get(list, i);
        char *key_type = map_get(kt_map, key_name);
        DataType *data_type = get_data_type(key_type);
        char *val = map_get(map, key_name);
        if (val != NULL) {
            errno = 0;
            data_type->cpy_to_memory(memory + offset, val);
            if (errno != 0) {
                if (errno == ERANGE) fprintf(stderr, "Out of range value for column \'%s\'", key_name);
                free(memory);
                return;
            }
        }
        else {
            //This branch should never be reached at this time
            //"Not null" is set default for any column at this time
            fprintf(stderr, "Column \'%s\' can't be null!\n", key_name);
            free(memory);
            return;
        }
        offset += data_type->get_type_size();
    }
    disk_pointer dp = dalloc(table->data);
    copy_to_disk(memory, offset, table->data, dp);
    free(memory);
    //key and dp will insert into a btree later
}

static void print_row(Table *table, void *memory) {
    size_t offset = 0;
    for (int i = 0; i < list_size(table->list); i++) {
        char *key = list_get(table->list, i);
        DataType *type = get_data_type(map_get(table->map, key));
        if (i) putchar(' ');
        type->print(memory + offset);    
        offset += type->get_type_size();
    }
    printf("\n");
}

void table_select(Table *table, KeyValueMap *example) {
    DISK *data = table->data;
    size_t block_size = data->block_size;
    disk_pointer dp = data_start_pos();
    static const size_t num_bytes_pre_IO = 4096;
    size_t num_blocks, num_blocks_read;
    if (block_size >= num_bytes_pre_IO)
        num_blocks = 1;
    else
        num_blocks = num_bytes_pre_IO / block_size;
    void *buffer = malloc(num_blocks * block_size);
    char **keys = (char **)malloc(map_size(example) * sizeof(char *));
    char **values = (char **)malloc(map_size(example) * sizeof(char *));
    map_get_all_keys_and_values(example, keys, values);
    
    while (1) {
        num_blocks_read = copy_to_memory_s(data, dp, num_blocks, buffer);
        if (num_blocks_read < 0) {
            fprintf(stderr, "Error in copy_to_memory_s\n");
            free(buffer);
            free(keys);
            free(values);
            return;
        }
        for (int i = 0; i < num_blocks_read; i++) {
            size_t offset = i * block_size;
            int flag = 1;
            for (int j = 0; j < map_size(example); j++) {
                DataType *type = get_data_type(map_get(table->map, keys[j]));                
                void *p_val = malloc(type->get_type_size());
                size_t key_offset = 0;
                for (int k = 0; k < list_size(table->list); k++) {
                    if (strcmp(list_get(table->list, k), keys[j]) == 0) break;
                    key_offset += type_size(map_get(table->map, list_get(table->list, k)));
                }
                memcpy(p_val, buffer + offset + key_offset, type->get_type_size());
                void *p_example_val = type->convert_to_val(values[j]);
                int compare_res = type->compare(p_val, p_example_val);                
                free(p_val);
                free(p_example_val);
                if (compare_res != 0) {
                    flag = 0;
                    break;
                }
            }
            if (flag) {
                print_row(table, buffer + offset);
            }
        }
        if (num_blocks_read < num_blocks) {
            break;
        }
        dp = next_n_pointer(data, dp, num_blocks);
    }

    free(buffer);
    free(keys);
    free(values);
}
