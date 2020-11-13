#include "disk.h"
#include <string.h>

int main() {
	DISK *disk = dcreate("./file", 1024);
	if (disk == NULL) exit(0);
	disk_pointer dp = dalloc(disk);
	if (dp == DNULL) {
		dclose(disk);
		exit(0);
	}
	char *str = "DESCRIPTION														\
       The  function  fread() reads nmemb items of data, each size bytes long,		\
       from the stream pointed to by stream,  storing  them  at  the  location		\
       given by ptr.																\
																					\
       The function fwrite() writes nmemb items of data, each size bytes long,		\
       to the stream pointed to by stream, obtaining them  from  the  location		\
       given by ptr.																\
																					\
       For nonlocking counterparts, see unlocked_stdio(3).							\
																					\
RETURN VALUE																		\
       On  success,  fread()  and  fwrite() return the number of items read or		\
       written.  This number equals the number of bytes transferred only  when		\
       size  is 1.  If an error occurs, or the end of the file is reached, the		\
       return value is a short item count (or zero).								\
																					\
       fread() does not distinguish between end-of-file and error, and callers		\
       must use feof(3) and ferror(3) to determine which occurred.";

	copy_to_disk(str, strlen(str), disk, dp);
	//free(str);
	dclose(disk);

	disk = dopen("./file");
	str = (char *)malloc(1024 * sizeof(char));
	copy_to_memory(disk, dp, str);
	printf("%s\n", str);

	free(str);
	dclose(disk);

	exit(0);
}
