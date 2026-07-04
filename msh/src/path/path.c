#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "path.h"
#include "env.h"



/*
 * parser_path
 *
 * Разбирает строку PATH (разделитель ':') и добавляет элементы в Vector.
 *
 * ПАМЯТЬ:
 * - каждый элемент создаётся через strdup → Vector становится владельцем строк
 * - освобождение выполняется через vec_free(path, free)
 * - при ошибке выполняется rollback: освобождаются только добавленные элементы
 *
 * ВАЖНО:
 * - пустые элементы ("::", ":start:", "end:") превращаются в "" через strdup
 * - входная строка не изменяется (работа идёт на копии)
 */
 
int parser_path(Vector *path, const char *str)
{
	if (!path || !str){
		errno = EINVAL;
		return -1;
	}
	
	size_t start = path->argc;
	
	char *copy = strdup(str);
	
	if (!copy){
		errno = ENOMEM;
		return -1;
	}
	
	char *s = copy;
	char *p = NULL;
	char *dup = NULL;
	
	while ((p = strchr(s, ':'))){
		*p = '\0';
			
		dup = strdup(s);
			
		if (!dup)
			goto err;
			
		if (vec_push(path, dup) < 0){
			free(dup);
			goto err;
		}
		
	s = p + 1;
	}
		
	dup = strdup(s);
		
	if (!dup)
		goto err;
		
	if (vec_push(path, dup) < 0){
		free(dup);
		goto err;
	}
	
	free(copy);
	return 0;
	
err:
	free(copy);
// откат к начальному состоянию
	while (path->argc > start){
		free(path->val[path->argc - 1]);
		path->argc--;
	}
	path->val[path->argc] = NULL;
	return -1;
}




// разбирает PATH на список имеющихся путей

int path_from_env(Vector *path, Vector *env)
{
	if (!path || !env){
		errno = EINVAL;
		return -1;
	}
	
	t_env *e = env_find(env, "PATH");
	
	if (!e || !e->value)
		return 0;
	
	Vector tmp;
	
	if(vec_init(&tmp, 0) < 0){
		errno = ENOMEM;
		return -1;
	}
	
	if (parser_path(&tmp, e->value) < 0){
		vec_free(&tmp, free);
		return -1;
	}
	
	vec_free(path, free);
	*path = tmp;
	
	return 0;
}