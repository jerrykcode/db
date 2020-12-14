#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../map.h"

#define TEST_NUM 100000 

int keys[TEST_NUM];
char *values[TEST_NUM];

char *itoa(int i) {
    int num = 0;
    int j = i;
    while (j) {
        j /= 10;
        num++;
    }
    char *str = malloc((num + 1)*sizeof(char));
    str[num] = '\0';
    while (i) {
        str[num - 1] = (i % 10) + '0';
        num--;
        i /= 10;
    }
    return str;
}

void make_data(void) {
    time_t t;
    srand((unsigned)time(&t));
    for (int i = 0; i < TEST_NUM; i++) {
        keys[i] = rand();
        values[i] = itoa(keys[i]);
    }
}

int cmp(const void *a, const void *b) {
    return *((int *)a) - *((int *)b);
}

void run_tests(void) {
    make_data();
    map_t *map = map_create(cmp, MAP_VALUE_SHALLOW_COPY);
    if (map == NULL) {
        fprintf(stderr, "FAILED!!!\n");
        exit(1);
    }
    map_set_key_size(map, sizeof(int));
    int ret;
    for (int i = 0; i < TEST_NUM; i++) {
        if (ret = map_put(map, &keys[i], values[i])) {
            fprintf(stderr, "FAILED!!! map_put %s\n", strerror(ret));
            map_destroy(map);
            exit(1);
        }
    }
    for (int i = 0; i < TEST_NUM; i++) {
        char *v = map_get(map, &keys[i]);
        if (v == NULL) {
            fprintf(stderr, "FAILED!!! map_get\n");
            map_destroy(map);
            exit(1);
        }
        if (strcmp(v, values[i])) {
            fprintf(stderr, "FAILED!!! map_get\n");
            map_destroy(map);
            exit(1);
        }
    }
    int *sorted_keys = malloc(TEST_NUM * sizeof(int));
    memcpy(sorted_keys, keys, TEST_NUM * sizeof(int));
    qsort(sorted_keys, TEST_NUM, sizeof(int), cmp);
	int **key_arr = malloc(TEST_NUM * sizeof(int *));
	map_sort(map, (void **)key_arr, NULL);
    for (int i = 0, j = 0; i < TEST_NUM; i++, j++) {
        while (i && sorted_keys[i] == sorted_keys[i - 1])
            i++;
//        printf("key: %d  rbtree: %d\n", sorted_keys[i], *key_arr[j]);
        if (sorted_keys[i] != *key_arr[j]) {
            fprintf(stderr, "FAILED!!! map_sort\n");
            map_destroy(map);
            exit(1);
		}
	}

    free(key_arr);
    free(sorted_keys);
	
	for (int i = 0; i < TEST_NUM; i++) {
        if (ret = map_remove(map, &keys[i])) {
            fprintf(stderr, "FAILED!!! map_remove\n");
            map_destroy(map);
            exit(1);
        }
//        printf("ok %d\n", i);
    }
    map_destroy(map);
}

int main() {
    
    run_tests();
    printf("test_map: All tests passed.\n"); 

    exit(0);
}
