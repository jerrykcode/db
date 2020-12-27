#include "disk.h"
#include <errno.h>

DISK *dopen(const char *pathname) {
	FILE *file = fopen(pathname, "r+");
	if (file == NULL) {
		perror("fopen()");
		return NULL;
	}
	DISK *disk = (DISK *)malloc(sizeof(DISK));
	disk->file = file;
	if (BLOCK_SIZE_OFFSET != 0) {
		if (fseek(file, BLOCK_SIZE_OFFSET, SEEK_SET) != 0) {
			perror("fseek()");
			fclose(file);
			free(disk);
			return NULL;
		}
	}
	size_t size = fread(&disk->block_size, BLOCK_SIZE_SIZE, 1, file);
	if (size != 1) {
		fclose(file);
		free(disk);
		return NULL;
	}
	return disk;
}

DISK *dcreate(const char *pathname, disk_t blocksize) {
	FILE *file = fopen(pathname, "w+");
	if (file == NULL) {
		perror("fopen()");
		return NULL;
	}
	DISK *disk = (DISK *)malloc(sizeof(DISK));
	disk->file = file;
	if (BLOCK_SIZE_OFFSET != 0) {
		if (fseek(file, BLOCK_SIZE_OFFSET, SEEK_SET) != 0) {
			perror("fseek()");
			fclose(file);
			free(disk);
			return NULL;
		}
	}
	size_t size = fwrite(&blocksize, BLOCK_SIZE_SIZE, 1, file);
	if (size != 1) {
		fclose(file);
		free(disk);
		return NULL;
	}
	disk->block_size = blocksize;
	return disk;
}

void dclose(DISK *disk) {
	if (disk->file != NULL) 
		fclose(disk->file);
	free(disk);
}

disk_pointer next_pointer(DISK *disk, disk_pointer dp) {
    return dp + disk->block_size;
}

disk_pointer next_n_pointer(DISK *disk, disk_pointer dp, size_t n) {
    return dp + n * disk->block_size;
}

disk_pointer data_start_pos() {
    return DATA_OFFSET;
}

disk_pointer dalloc(DISK *disk) {
	FILE *file = disk->file;
	long fp_save = ftell(file); //save the file position when this function begin
	fseek(file, 0, SEEK_END);   //set the file position to the end
	disk_pointer dp = (disk_pointer)ftell(file);
	fseek(file, disk->block_size, SEEK_END); //alloc new space
	fseek(file, fp_save, SEEK_SET); //set the file position back
	return dp;
}

int dalloc_first_block(DISK *disk) {
    FILE *file = disk->file;
    long fp_save = ftell(file); //save the file position when this function begin
    fseek(file, 0, SEEK_END);
    if ((disk_pointer)ftell(file) == data_start_pos()) {
    	fseek(file, disk->block_size, SEEK_END); //alloc new space
        fseek(file, fp_save, SEEK_SET); //set the file position back
        return 0; 
    }
    else {
        fseek(file, fp_save, SEEK_SET); //set the file position back
        return EINVAL;
    }
}

disk_pointer first_block(DISK *disk) {
    return data_start_pos(); 
}

int copy_to_memory_s(DISK *disk, disk_pointer src, size_t num_blocks, void *des) {
    FILE *file = disk->file;
    long fp_save = ftell(file);
    long offset = src;
    fseek(file, offset, SEEK_SET);
    size_t num_blocks_read = fread(des, disk->block_size, num_blocks, file);
    if (num_blocks_read != num_blocks) {
        if (!feof(file)) return -1;
    } 
    fseek(file, fp_save, SEEK_SET);
    return num_blocks_read;
}

int copy_to_memory(DISK *disk, disk_pointer src, void *des) {
    return copy_to_memory_s(disk, src, 1, des);
}

int copy_to_disk(void *src, size_t size, DISK *disk, disk_pointer des) {
	FILE *file = disk->file;
	long offset = des;
	fseek(file, offset, SEEK_SET);
	if (size > disk->block_size) size = disk->block_size;
	size_t num_blocks_wrote = fwrite(src, size, 1, file);
	if (num_blocks_wrote != 1) {
		return -1;
	}
	return 1;
}
