#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "io.h"
#include "buf.h"


#ifdef LINE_MAX
	#define BUF_SIZE LINE_MAX
#elif BUFSIZ
	#define BUF_SIZE BUFSIZ
#else
	#define BUF_SIZE 512
#endif


/* Функция читает строку произвольной длины из потока.
 *
 * Возвращает:
 *  - строку (нужно free)
 *  - NULL при EOF или ошибке
 *
 * flg — битовая маска:
 * RL_SHELL — режим shell (пустая строка допустима и возвращается как "")
 */
 
char *read_line(FILE *fp, int flg)
{
	if(!fp){
		errno = EINVAL;
		return NULL;
	}

	char *str = NULL;
	char *tmp = NULL;
	size_t init = BUF_SIZE; // начальный размер буфера
	
	// Выделяем память
	str = malloc(init);
	if (!str)
		return NULL;

	// Читаем первые данные
	if (!fgets(str, init, fp))
	{
		if (ferror(fp)){
			if (errno == 0)
				errno = EIO;
		} else errno = 0;
		
		free(str);
		return NULL;
	}
	
	size_t len = strlen(str);

		// Расширяем буфер, если строка не уместилась
	while (len > 0 && str[len - 1] != '\n'){
		
		if(init >= SIZE_MAX / 2){
			free(str);
			errno = ENOMEM;
			return NULL;
		}
		init *= 2;
		tmp = realloc(str, init);
		if(!tmp){
			errno = ENOMEM;
			free(str);
			return NULL;
		}
		str = tmp;

		if (!fgets(str + len, init - len, fp)){
			if (ferror(fp)){
				if(errno == 0)
					errno = EIO;
				free(str);
				return NULL;
			}
			break;
		}

		size_t chunk = strlen(str + len);
		len += chunk;
	}

		// Поджимаем память, если выделено слишком много
	if (len + 1 < init / 2){
		if ((tmp = realloc(str, len + 1)))
			str = tmp;
	}

		// Убираем финальный '\n', если есть
	len = strcspn(str, "\r\n");
	str[len] = '\0';

		// Проверка пустой строки (режим shell)
	if ((flg & RL_SHELL) && str[0] == '\0')
		return str;
		
	return str;
}




/*
* пишет из указаного буфера в указаный файловый дескриптор
* пишет буфер полностью
* возвращает
-1 при ошибке
0 при успехе
*/

int puts_fd(int out_fd, const char *buf, size_t len)
{
	if (out_fd < 0 || !buf){
		errno = EINVAL;
		return -1;
	}
	
	if (len == 0)
		len = strlen(buf);
	size_t off = 0;
	
	while (off < len){
		
		ssize_t w = write(out_fd, buf + off, len - off);
		
		if (w == 0){
			errno = EIO;
			return -1;
		}
		
		if (w < 0){
			if (errno == EINTR)
				continue;
			return -1;
		}
		
		off += (size_t) w;
	}
	return 0;
}


/*
* формирует строку и выводит ее в форматном виде в указаный файловый дескриптор
*/

int printf_fd(int out_fd, const char *fmt, ...)
{
	if (out_fd < 0 || !fmt){
		errno = EINVAL;
		return -1;
	}
	
	va_list ap;
	va_start(ap, fmt);
	int need = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	
	if (need < 0)
		return -1;
	
	if ((size_t) need > SIZE_MAX - 1){
		errno = ENOMEM;
		return -1;
	}
	
	char *buf = malloc((size_t) need + 1);
	
	if (!buf)
		return -1;
	
	va_start(ap, fmt);
	int written = vsnprintf(buf, need + 1, fmt, ap);
	va_end(ap);
	
	if (written < 0){
		free(buf);
		return -1;
	}
	
	int r = puts_fd(out_fd, buf, written);
	
	free(buf);
	
	return r;
}




/* читает содержимое fd целиком и возвращает строку */

char *read_fd(int in_fd, size_t *sizep)
{
	if (in_fd < 0){
		errno = EINVAL;
		return NULL;
	}
	
	t_buf buf;
	ssize_t n;
	char tmp[BUF_SIZE];
	
	if (buf_init(&buf) < 0)
		return NULL;
	
	while (1){
		
		n = read(in_fd, tmp, sizeof(tmp));
		
		if (n > 0){
			
			if (buf_append_span(&buf, tmp, (size_t) n) < 0){
				buf_free(&buf);
				return NULL;
			}
			
			continue;
		}
		
		if (n == 0)
			break;
		
		if (n == -1 && errno == EINTR)
			continue;
		
		buf_free(&buf);
		return NULL;
	}
	
	if (sizep)
		*sizep = buf.len;
	
	return buf_finalize(&buf);
}