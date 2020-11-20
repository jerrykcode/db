#ifndef DATATYPE_H__
#define DATATYPE_H__

//The data types will later be replaced by enum
size_t type_size(const char *type) {
	if (strcmp(type, "int") == 0) {
		return sizeof(int);
	}
	else if (strcmp(type, "bigint") == 0) {
		return sizeof(long long);
	}
	return 0;
}

int is_valid_datatype(const char *type) {
    return (strcmp(type, "int") == 0) || (strcmp(type, "bigint") == 0);
}

#endif
