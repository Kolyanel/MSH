#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "my_string.h"
#include "memory.h"
#include "vector.h"


/*
* начальная инициализация массива
указателей на строки
argv подобный
* начальный размер, если не указан == 8 элементам
с NULL терминатором
*/

char ** init_charrp(size_t cnt)
{
	
	size_t cap = 8;
	cnt = cnt ? cnt : cap;
	
	char **arr = malloc((cnt + 1) * sizeof(*arr));
	
	if (!arr){
		errno = ENOMEM;
		return NULL;
	}
	
	for (size_t i = 0; i < cnt; ++i)
		arr[i] = NULL;
	
	arr[cnt] = NULL;
	
	return arr;
}




// функция проверяет есть ли строка val в строке str
// исходная строка остается не измененной
// параметр sep это разделитель по которому разбивается исходная строка

bool strlist_has(const char *str, const char *val, const char *sep)
{
	if (!str || !val || !*val || !sep || !*sep){
		errno = EINVAL;
		return false;
	}
	
	if (strcmp(str, val) == 0)
		return true;
	
	char *copy = strdup(str);
	if (!copy)
		return false;
	
	char *ptr = copy;
	char *token;
	
	while ((token = strsep(&ptr, sep))){
		if (strcmp(token, val) == 0){
			free(copy);
			return true;
		}
	}
	
	free(copy);
	return false;
}



// функция собирает в одну 2е строки
// возвращает указатель на новую строку с результатом
// параметр sep это разделитель строк, если его нет в конце str1, добавляет его между этими строками
// fn_alloc:
//  - выделяет буфер
//  - записывает его размер в *size
//  - гарантирует, что размер достаточен для размещения результата
//  - возвращает NULL при ошибке и устанавливает errno
// функция сама выделяет память для новой строки
// надо делать free в вызывающей функции

char *cat_str_sep(const char *str1, const char *str2, char sep, char *(*fn_alloc)(size_t *size))
{
	if (!str1 || !str2 || !fn_alloc){
		errno = EINVAL;
		return NULL;
	}
	
	size_t size = strlen(str1) + strlen(str2) + 2;
	
	char *res = fn_alloc(&size);
	
	if (!res)
		return NULL;
	
	size_t len = strlen(str1);
	char sep_str[2] = {sep, '\0'};
	
	const char *s = ( len > 0 && str1[len - 1] == sep) ? "" : sep_str;
	
	int ret = snprintf(res, size, "%s%s%s", str1, s, str2);
	if (ret < 0 || (size_t) ret >= size){
		errno = ENAMETOOLONG;
		free(res);
		return NULL;
	}
	return res;
}



/*
 * parser_str - разбивает строку str на массив токенов по sep
 * Возвращает NULL при ошибке (errno установлен).
 * Массив можно использовать как argv: NULL-терминатор есть.
 * исходная str не меняется.
 */
 
char **parser_str(const char *str, const char *sep)
{
	if (!str || !sep){
		errno = EINVAL;
		return NULL;
	}
	
	Vector v;
	
	if ((vec_init(&v, 0)) < 0)
		return NULL;
	
	char *copy = strdup(str);
	
	if (!copy){
		vec_free(&v, free);
		return NULL;
	}
	
	char *s = copy;
	char *p;
	char *dup;
	
	while ((p = strpbrk(s, sep))){
		if(p != s){
			
			*p = '\0';
			
			dup = strdup(s);
			
			if (!dup)
				goto err_dup;
			
			if (vec_push(&v, dup) < 0)
				goto err_push;
		}
		
		s = p + 1;
	}
	
	if (*s){
		
		dup = strdup(s);
		
		if (!dup)
			goto err_dup;
		
		if (vec_push(&v, dup) < 0)
			goto err_push;
	}
	
	free(copy);
	return (char**)v.val;
	
err_dup:
	free(copy);
	vec_free(&v, free);
	errno = ENOMEM;
	return NULL;

err_push:
	free(dup);
	free(copy);
	vec_free(&v, free);
	errno = ENOMEM;
	return NULL;
}




// освобождение массива аргументов

void free_args(char **argv)
{
	if (!argv) return;
	
	char **tmp = argv;
	while (*tmp)
		free(*tmp++);
	
	free(argv);
}



/*
 * free_arrp - освобождение массива строк и его элементов
 * arr - массив строк, cnt - количество элементов
 */
 
void free_arrp(char **arr, size_t cnt)
{
	
	if (!arr)
		return;
	
	for (size_t i = 0; i < cnt; ++i)
		free(arr[i]);
	
	free(arr);
}



/* простая конкатенация строк */

char *str_join(const char *a, const char *b)
{
	if (!a || !b){
		errno = EINVAL;
		return NULL;
	}
	
	size_t len_a = strlen(a),
	len_b = strlen(b);
	
	size_t tot_len = len_a + len_b;
	
	char *buf = malloc(tot_len + 1);
	
	if (!buf)
		return NULL;
	
	memcpy(buf, a, len_a);
	memcpy(buf + len_a, b, len_b);
	
	buf[tot_len] = '\0';
	
	return buf;
}




/* глубокое копирование
указателя на массив указателей*/

char **copy_argv(char **argv)
{
	if (!argv){
		errno = EINVAL;
		return NULL;
	}
	
	size_t cnt = 0;
	
	while (argv[cnt])
		cnt++;
	
	char **dst = calloc(cnt + 1, sizeof(char*));
	
	if (!dst)
		return NULL;
	
	for (size_t i = 0; i < cnt; ++i){
		
		dst[i] = strdup(argv[i]);
		
		if (!dst[i]){
			
			for(size_t j = 0; j < i; ++j)
				free(dst[j]);
			free(dst);
			return NULL;
		}
	}
	
	dst[cnt] = NULL;
	
	return dst;
}