#include "../disk.h"
#include <string.h>

int main() {
	DISK *disk = dcreate("./file", 1024);
	if (disk == NULL) exit(0);
	disk_pointer dp = dalloc(disk);
	if (dp == DNULL) {
		dclose(disk);
		exit(0);
	}
	char *str = 
"This manual page documents version 5.38 of the file command.\n\
file tests each argument in an attempt to classify it.  There are three\n\
sets of tests, performed in this order: filesystem tests, magic tests,\n\
and language tests.  The first test that succeeds causes the file type\n\
to be printed.\n\
The type printed will usually contain one of the words text (the file\n\
contains only printing characters and a few common control characters\n\
and is probably safe to read on an ASCII terminal), executable (the file\n\
contains the result of compiling a program in a form understandable to\n\
some UNIX kernel or another), or data meaning anything else (data is\n\
usually “binary” or non-printable).  Exceptions are well-known file for‐\n\
mats (core files, tar archives) that are known to contain binary data.\n\
When adding local definitions to /etc/magic, make sure to preserve these\n\
keywords.  Users depend on knowing that all the readable files in a di‐\n\
rectory have the word “text” printed.  Don't do as Berkeley did and\n\
change “shell commands text” to “shell script”.\n\
The filesystem tests are based on examining the return from a stat(2)\n\
system call.  The program checks to see if the file is empty, or if it's\n\
some sort of special file.  Any known file types appropriate to the sys‐\n\
tem you are running on (sockets, symbolic links, or named pipes (FIFOs)\n\
on those systems that implement them) are intuited if they are defined\n\
in the system header file <sys/stat.h>.\n";


	copy_to_disk(str, strlen(str), disk, dp);
	dclose(disk);

    disk = dopen("./file");
    char *file_str = (char *)malloc(disk->block_size);
    copy_to_memory(disk, dp, file_str);
    size_t len = strlen(file_str);
    dp = dalloc(disk);
    copy_to_disk(str + len, strlen(str) - len, disk, dp);
    dclose(disk);
    free(file_str);

	exit(0);
}
