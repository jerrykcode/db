#include "table.h"
#include "datatype.h"
#include "frame.h"
#include "btree.h"
#include <errno.h>

static char *get_data_pathname(const char *path, const char *table_name) {
    size_t path_len = strlen(path);
    size_t table_name_len = strlen(table_name);
    size_t data_suffix_len = strlen(DATA_SUFFIX);
    size_t EOF_SIZE = 1; //space for '\0'
    char *data_pathname = (char *)malloc(path_len + table_name_len + data_suffix_len + EOF_SIZE);
    strcpy(data_pathname, path);
    strcpy(data_pathname + path_len, table_name);
    strcpy(data_pathname + path_len + table_name_len, DATA_SUFFIX);
    return data_pathname;
}

static char *get_frm_pathname(const char *path, const char *table_name) {
    size_t path_len = strlen(path);
    size_t table_name_len = strlen(table_name);
    size_t frm_suffix_len = strlen(FRAME_SUFFIX);
    size_t EOF_SIZE = 1; //space for '\0'
    char *frm_pathname = (char *)malloc(path_len + table_name_len + frm_suffix_len + EOF_SIZE);
    strcpy(frm_pathname, path);
    strcpy(frm_pathname + path_len, table_name);
    strcpy(frm_pathname + path_len + table_name_len, FRAME_SUFFIX);
    return frm_pathname;
}

static void *cpy_to_buffer(const char *table_name, ColNameList *list, map_t *index2btree, ColNameTypeMap *map, size_t *p_buffersize, size_t *p_blocksize) {
    size_t table_name_size = strlen(table_name);
    if (table_name_size > FRM_TABLE_NAME_SIZE) {
        fprintf(stderr, "Table name:\'%s\' too long", table_name);
        return NULL;
    }
    size_t num_cols = list_size(list);
    *p_buffersize = FRM_SIZE(num_cols);
    void *buffer = malloc(*p_buffersize);
    memcpy(buffer + FRM_TABLE_NAME_OFFSET, (void *)table_name, table_name_size);
    memcpy(buffer + FRM_NUM_COLS_OFFSET, (void *)(&num_cols), FRM_NUM_COLS_SIZE);
    *p_blocksize = 0;
    for (int i = 0; i < num_cols; i++) {
        char *name = list_get(list, i);
        char *type = map_get(map, name);
        size_t name_size = strlen(name);
        if (name_size > FRM_COL_NAME_SIZE || !is_valid_datatype(type)) {
            if (name_size > FRM_COL_NAME_SIZE)
                fprintf(stderr, "Column name: \'%s\' too long!\n", name);
            else
                fprintf(stderr, "Column: \'%s\' with invalid data type: \'%s\'\n", name, type);
            free(buffer);
            return NULL;
        }
        memcpy(buffer + FRM_COL_NAME_OFFSET(i), name, name_size);
        memcpy(buffer + FRM_COL_TYPE_OFFSET(i), type, strlen(type));
        u_int8_t flag_is_index;
        if (map_has_key(index2btree, name))
            flag_is_index = 1;
        else
            flag_is_index = 0;
        memcpy(buffer + FRM_COL_INDEX_FLAG_OFFSET(i), &flag_is_index, sizeof(flag_is_index));
         
        *p_blocksize += type_size(type);
    }
    return buffer;
}

static function_ColNameTypeMap_compare_key(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

Table *table_create(const char *path, const char *table_name, ColNameList *list, List *indices, ColNameTypeMap *map) {
    
    map_t *index2btree = map_create(function_ColNameTypeMap_compare_key, MAP_KEY_REFERENCE_COPY | MAP_VALUE_REFERENCE_COPY);
    for (int i = 0; i < list_size(indices); i++) {
        char *col_name = list_get(indices, i);
        map_put(index2btree, col_name, btree_create(path, table_name, col_name, get_data_type(map_get(map, col_name))));
    }

    size_t buffer_size, block_size;
    void *buffer = cpy_to_buffer(table_name, list, index2btree, map, &buffer_size, &block_size);
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
    table->index2btree = index2btree;
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
    fseek(frm, FRM_NUM_COLS_OFFSET, SEEK_SET);
    size_t num_cols;
    fread(&num_cols, sizeof(size_t), 1, frm);
    size_t buffer_size = FRM_SIZE(num_cols);
    void *buffer = malloc(buffer_size);
    fseek(frm, 0, SEEK_SET);
    fread(buffer, buffer_size, 1, frm);
    fclose(frm);
    ColNameList *list = new_list();
    ColNameTypeMap *map = map_create(function_ColNameTypeMap_compare_key, MAP_KEY_REFERENCE_COPY | MAP_VALUE_SHALLOW_COPY);
    map_t *index2btree = map_create(function_ColNameTypeMap_compare_key, MAP_KEY_REFERENCE_COPY | MAP_VALUE_REFERENCE_COPY);
    size_t block_size;
    for (int i = 0; i < num_cols; i++) {
        char *name = (char *)malloc(FRM_COL_NAME_SIZE);
        memcpy(name, buffer + FRM_COL_NAME_OFFSET(i), FRM_COL_NAME_SIZE);
        list_add(list, name);
        char *type = (char *)malloc(FRM_COL_TYPE_SIZE);
        memcpy(type, buffer + FRM_COL_TYPE_OFFSET(i), FRM_COL_TYPE_SIZE);
        map_put(map, name, type);
        u_int8_t flag_is_index;
        memcpy(&flag_is_index, buffer + FRM_COL_INDEX_FLAG_OFFSET(i), FRM_COL_INDEX_FLAG_SIZE);
        if (flag_is_index)
            map_put(index2btree, name, btree_open(path, table_name, name, get_data_type(type)));
    } 
    free(buffer);
    char *data_pathname = get_data_pathname(path, table_name);
    DISK *data = dopen(data_pathname);
    free(data_pathname);
    if (data == NULL) {
        fprintf(stderr, "error in dopen()!\n");
        for (int i = 0; i < num_cols; i++)
            free(list_get(list, i));
        list_free(list);
        map_destroy(map);
        return NULL;
    }
    Table *table = (Table *)malloc(sizeof(Table));
    table->map = map;
    table->list = list;
    table->index2btree = index2btree;
    table->data = data;
    return table;
}

void table_close(Table *table) {
    List *list = table->list;
    size_t num_cols = list_size(list);
    map_t *map = table->map;
    for (int i = 0; i < num_cols; i++)
        free(list_get(list, i));
    list_free(list);
    map_destroy(map);
    map_t *index2btree = table->index2btree;
    PBTree *btrees = (PBTree *)malloc(map_size(index2btree) * sizeof(PBTree));
    map_sort(index2btree, NULL, btrees); //replace by map iterator later
    for (int i = 0; i < map_size(index2btree); i++)
        btree_close(btrees[i]);
    free(btrees);
    map_destroy(index2btree);
    dclose(table->data);
    free(table);
}

void table_insert(Table *table, ColNameValueMap *map) {
    size_t block_size = table->data->block_size;
    void *memory = malloc(block_size);
    ColNameList *list = table->list;
    ColNameTypeMap *nt_map = table->map;
    size_t offset = 0;
    for (int i = 0; i < list_size(list); i++) {
        char *col_name = list_get(list, i);
        char *col_type = map_get(nt_map, col_name);
        DataType *data_type = get_data_type(col_type);
        char *val = map_get(map, col_name);
        if (val != NULL) {
            errno = 0;
            data_type->cpy_to_memory(memory + offset, val);
            if (errno != 0) {
                if (errno == ERANGE) fprintf(stderr, "Out of range value for column \'%s\'", col_name);
                free(memory);
                return;
            }
        }
        else {
            //This branch should never be reached at this time
            //"Not null" is set default for any column at this time
            fprintf(stderr, "Column \'%s\' can't be null!\n", col_name);
            free(memory);
            return;
        }
        offset += data_type->get_type_size();
    }
    disk_pointer dp = dalloc(table->data);
    copy_to_disk(memory, offset, table->data, dp);
    free(memory);
    

    //replace by map iterator later
    map_t *index2btree = table->index2btree;
    size_t num_indices = map_size(index2btree);
    PBTree *btrees = (PBTree *)malloc(num_indices * sizeof(PBTree));
    char **names = (char **)malloc(num_indices * sizeof(char *));    
    map_sort(index2btree, names, btrees); 
    for (int i = 0; i < map_size(index2btree); i++) {
        void *key = get_data_type(map_get(table->map, names[i]))->convert_to_val(map_get(map, names[i]));
        btree_insert(btrees[i], key, dp);
    }
    free(btrees);
    free(names);
}

static void print_row(Table *table, void *memory) {
    size_t offset = 0;
    for (int i = 0; i < list_size(table->list); i++) {
        char *col_name = list_get(table->list, i);
        DataType *type = get_data_type((char *)map_get(table->map, col_name));
        if (i) putchar(' ');
        type->print(memory + offset);    
        offset += type->get_type_size();
    }
    printf("\n");
}

static void table_select_noindex(Table *table, ColNameValueMap *example) {
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
    map_sort(example, (void **)keys, (void **)values);
    
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
                DataType *type = get_data_type((char *)map_get(table->map, keys[j]));
                void *p_val = malloc(type->get_type_size());
                size_t key_offset = 0;
                for (int k = 0; k < list_size(table->list); k++) {
                    if (strcmp(list_get(table->list, k), keys[j]) == 0) break;
                    key_offset += type_size((char *)map_get(table->map, list_get(table->list, k)));
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

void table_select(Table *table, ColNameValueMap *example) {
    //Just a simple test for btree
    //Improve this function later
    char **keys = (char **)malloc(map_size(example) * sizeof(char *));
    char **values = (char **)malloc(map_size(example) * sizeof(char *));
    map_sort(example, (void **)keys, (void **)values);
    
    void *ek = get_data_type(map_get(table->map, keys[0]))->convert_to_val(values[0]);
    vector_t *dps = btree_select(map_get(table->index2btree, keys[0]), ek, ek);
    free(ek);
    DISK *data = table->data;
    void *buffer = malloc(data->block_size);
    for (int i = 0; i < vector_size(dps); i++) {
        copy_to_memory(data, *(disk_pointer *)vector_get(dps, i), buffer);
        print_row(table, buffer);
    }
    free(buffer);
}
