#ifndef DISK_H__
#define DISK_H__

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE_OFFSET 0
typedef unsigned int disk_t;
#define BLOCK_SIZE_SIZE sizeof(disk_t)

typedef struct {
	FILE *file;
	disk_t block_size;
} DISK;

DISK *dopen(const char *pathname);
DISK *dcreate(const char *pathname, disk_t blocksize);
void dclose(DISK *disk);

typedef disk_t disk_pointer;
#define DNULL 0x0

disk_pointer next_pointer(DISK *disk, disk_pointer dp);
disk_pointer dalloc(DISK *disk);
int copy_to_memory(DISK *disk, disk_pointer src, void *des);
int copy_to_disk(void *src, size_t size, DISK *disk, disk_pointer des);

#endif
