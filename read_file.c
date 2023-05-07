/*
   Copyright (c) 2023 - Alfonso Landín

   Permission is granted for use and modification of this file for
   research, non-commercial purposes.
*/

#include "read_file.h"
#include "common.h"
#include "sysfunc.h"
#include <zstd.h>
#include <zlib.h>

#define GZBUFFER_SIZE (128 * 1024)

int
ends_with (const char *str, const char *suffix)
{
    int str_length = strlen(str);
    int suffix_length = strlen(suffix);
    if (str_length < suffix_length) {
	return (UNDEF);
    }

    int result = strncmp(str + str_length - suffix_length, suffix, suffix_length);
    return !result;
}

int
read_file_open (char *file_path, char **dest, unsigned int *size)
{
    FILE *fd;
    if (!(fd = fopen(file_path, "rb")) ||
	fseek(fd, 0L, SEEK_END) != 0 || 0 >= (*size = ftell(fd)) ||
	NULL == (*dest = malloc(*size + 2)) ||
	-1 == fseek(fd, 0L, SEEK_SET) ||
	*size != fread(*dest, 1, *size, fd) ||
	-1 == fclose(fd))
    {
	return (UNDEF);
    }
    return 0;
}

int
read_file_gzip (char *file_path, char **dest, unsigned int *size)
{
    FILE *fd;
    gzFile gz;
    if (!(fd = fopen(file_path, "rb")) ||
	-1 == fseek(fd, -sizeof(*size), SEEK_END) ||
	1 != fread(size, sizeof(*size), 1, fd) ||
	-1 == fclose(fd))
    {
	return (UNDEF);
    }
    if (NULL == (*dest = malloc(*size + 2)) ||
    	NULL == (gz = gzopen(file_path, "rb"))||
        -1 == gzbuffer(gz, GZBUFFER_SIZE) ||
	*size != gzfread(*dest, 1, *size, gz) ||
	Z_OK != gzclose(gz))
    {
	return (UNDEF);
    }
    return 0;
}

int
read_file_zstd(char *file_path, char **dest, unsigned int *size) {
    unsigned int c_size;
    char *c_buf;
    if (UNDEF == read_file_open(file_path, &c_buf, &c_size)) {
	free(c_buf);
	return(UNDEF);
    }
    if (ZSTD_CONTENTSIZE_ERROR == (*size = ZSTD_getFrameContentSize(c_buf, c_size)) ||
	ZSTD_CONTENTSIZE_UNKNOWN == *size ||
	NULL == (*dest = malloc(*size + 2)) ||
	*size != ZSTD_decompress(*dest, *size, c_buf, c_size))
    {
	free(c_buf);
	return(UNDEF);
    }
    free(c_buf);
    return 0;
}

int
read_file (char *file_path, char **dest)
{
    unsigned int size = 0;
    int result;
    if (ends_with(file_path, ".gz")) {
	result = read_file_gzip(file_path, dest, &size);
    } else if (ends_with(file_path, ".zst")) {
	result = read_file_zstd(file_path, dest, &size);
    } else {
	result = read_file_open(file_path, dest, &size);
    }
    if (UNDEF != result) {
	/* Append ending newline if not present, Append NULL terminator */
	if ((*dest)[size - 1] != '\n')
	{
	    (*dest)[size] = '\n';
	    size++;
	}
	(*dest)[size] = '\0';
    }
    return result;
}
