#ifndef DISK_H__
#define DISK_H__

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE_OFFSET 0
typedef unsigned int disk_t;
#define BLOCK_SIZE_SIZE sizeof(disk_t)
#define DATA_OFFSET (BLOCK_SIZE_OFFSET + BLOCK_SIZE_SIZE)

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
disk_pointer next_n_pointer(DISK *disk, disk_pointer dp, size_t n);

/* Returns the starting position of data. */
disk_pointer data_start_pos();

/* Allocs disk space of one block_size and returns the pointer of the space. */
disk_pointer dalloc(DISK *disk);

/* Copies data of several block_sizes from disk area 'src' to memory area 'des'.
   The number of block_sizes is indicated by the parameter 'num_blocks'. 
   If there are no 'num_blocks' blocks of data left in the disk, then only 
   the data left in the disk are copied.
   The actual number of blocks copied is returned on success, otherwise a
   negative number is returned. */
int copy_to_memory_s(DISK *disk, disk_pointer src, size_t num_blocks, void *des);
/* Copies data of one block_size from disk area 'src' to memory area 'des'. 
   This is same as copy_to_memory_s(disk, src, 1, des) */
int copy_to_memory(DISK *disk, disk_pointer src, void *des);

/* Copies data of 'size' bytes from memory area 'src' to disk area 'des'. 
   If 'size' is larger than one block_size, then the memory is truncated. */
int copy_to_disk(void *src, size_t size, DISK *disk, disk_pointer des);

#endif
