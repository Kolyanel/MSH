#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

#define BUF_SIZE 4096
#define PATH_MAX_GUESS 1024




// Динамическое выделение размера памяти для буфера

char *buf_alloc(int fd, size_t *sizep);



// Динамическое выделение памяти для пути и имени файла

char *path_alloc(size_t *sizep);



// функция обертка для malloc

char *malloc_wrap(size_t *size);

#endif // MEMORY_H