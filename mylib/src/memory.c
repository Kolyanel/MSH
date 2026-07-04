#define _GNU_SOURCE

#include "memory.h"

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

// если системно определен размер пути мы берем его

#ifdef PATH_MAX
	static long pathmax = PATH_MAX;
// если нет, будем его вычислять автоматически
#else
	static long pathmax = 0;
#endif

// версии системы
static long posix_version = 0;
static long xsi_version = 0;
// размер буфера
static long bufsize;


// динамическое выделение памяти под строку пути имени

char* path_alloc(size_t* sizep)
{
	char* ptr;
	size_t size;
	
	if (posix_version == 0)
		posix_version = sysconf(_SC_VERSION);
	
	if (xsi_version == 0)
		xsi_version = sysconf(_SC_XOPEN_VERSION);
	
	if (pathmax == 0){
		errno = 0;
		if ((pathmax = pathconf("/", _PC_PATH_MAX)) <0){
			if (errno == 0)
				pathmax = PATH_MAX_GUESS;
			else return NULL;
		} else pathmax++;
	}
	if ((posix_version < 200112L) && (xsi_version < 4))
		size = pathmax + 1;
	else
		size = pathmax;
	
	if (!(ptr = malloc(size))){
		errno = ENOMEM;
		return NULL;
	}
	
	if (sizep)
		*sizep = size;
	return ptr;
}



// Динамическое выделение памяти под буфер

char *buf_alloc(int fd, size_t *sizep)
{
// указатель на выделеную память
	char* ptr;
// структура из поля которой используя fstat будем пытаться взять размер блока в системе
	struct stat st;
	
// если размер буфера еще неопределен получаем его
	if (bufsize == 0){
		errno = 0;
		if (fd >= 0 && fstat(fd, &st) == 0 && st.st_blksize > 0)
			bufsize = (long) st.st_blksize;
		else {
			if ((bufsize = sysconf(_SC_PAGE_SIZE)) < 0){
				if (errno == 0)
					bufsize = BUF_SIZE;
				else return NULL;
			}
		}
	}
	if (!(ptr = malloc(bufsize + 1))){
		errno = ENOMEM;
		return NULL;
	}
	if (sizep)
		*sizep = bufsize;
	return ptr;
}




// функция обертка для malloc

char *malloc_wrap(size_t *size)
{
	if (!size){
		errno = EINVAL;
		return NULL;
	}
	char *p = malloc(*size);
	return p ? p : NULL;
}