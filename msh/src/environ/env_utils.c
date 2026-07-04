#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "env_utils.h"



// Разделяет строку вида "KEY=VALUE" на key и val
// - если '=' нет, val = NULL
// - выделяет память для key и val через strdup/strndup
// - возвращает 0 при успехе, -1 при ошибке (например, ошибка аллокации)

int parse_assignment(const char *arg, char **key, char **val, int *mode)
{
	if (!arg || !key || !val || !mode){
		errno = EINVAL;
		return -1;
	}
	
	*key = NULL;
	*val = NULL;
	
	const char *eq = strchr(arg, '=');
	
	if (!eq){
		errno = EINVAL;
		return -1;
	}
	
	int is_append = (eq > arg && *(eq - 1) == '+');
	
	size_t len_key = is_append ? (size_t) ((eq - 1) - arg) : (size_t) (eq - arg);
	
	if (len_key == 0){
		errno = EINVAL;
		return -1;
	}
		
	*key = strndup(arg, len_key);
		
	if (!*key){
		errno = ENOMEM;
		return -1;
	}
		
	if (!is_valid_identifier(*key)){
		free(*key);
		*key = NULL;
		errno = EINVAL;
		return -1;
	}
	
	*val = strdup(eq + 1);
	
	if (!*val){
		errno = ENOMEM;
		free(*key);
		*key = NULL;
		return -1;
	}
	
	*mode = is_append ? APPEND : ASSIGN;
	
	return 0;
}



// Проверяет, что строка s является допустимым идентификатором переменной среды
// - начинается с буквы или '_'
// - далее могут быть буквы, цифры или '_'
// Возвращает 1 если корректно, 0 если нет

int is_valid_identifier(const char *s)
{
	if (!s || !*s)
		return 0;
	
	if (!(isalpha((unsigned char)*s) || *s == '_'))
		return 0;
	
	while (*++s){
		if (!(isalnum((unsigned char)*s) || *s == '_'))
			return 0;
	}
	
	return 1;
}