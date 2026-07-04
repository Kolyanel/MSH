#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "vector.h"


/*
 * vec_init - инициализация динамического массива указателей (Vector)
 *
 * Примечание:
 *   память освобождается через vec_free
 */
 
int vec_init(Vector *v, size_t cap)
{
	if (!v){
		errno = EINVAL;
		return -1;
	}
	
	if (cap == 0)
		cap = 8;
	
	if (cap + 1 > SIZE_MAX / sizeof(*v->val)){
		errno = ENOMEM;
		return -1;
	}
	
	v->val = malloc((cap + 1) * sizeof(*v->val));
	
	if (!v->val){
		errno = ENOMEM;
		v->cap = 0;
		v->argc = 0;
		return -1;
	}
	
	v->cap = cap;
	v->argc = 0;
	v->val[0] = NULL;
	
	return 0;
}




/*
 * vec_push - добавляет элемент в Vector
 *
 */
 
int vec_push(Vector *v, void *item)
{
	
	if (!v || !v->val){
		errno = EINVAL;
		return -1;
	}
	
	if (v->argc >= v->cap){
		
		size_t new_cap = v->cap ? v->cap * 2 : 8;
		
		if (new_cap + 1 > SIZE_MAX / sizeof(*v->val)){
			errno = ENOMEM;
			return -1;
		}
		
		void **tmp = realloc(v->val, (new_cap + 1) * sizeof(*v->val));
		
		if (!tmp){
			errno = ENOMEM;
			return -1;
		}
		
		v->val = tmp;
		v->cap = new_cap;
	}

	v->val[v->argc++] = item;
	
	v->val[v->argc] = NULL;
	
	return 0;
}




/*
 * vec_free - освобождает память Vector
 *
 */
 
void vec_free(Vector *v, void (*free_fn)(void*))
{
	if (!v){
		return;
	}
	
	if (v->val){
		if (free_fn){
			for (size_t i = 0; i < v->argc; ++i)
				free_fn(v->val[i]);
		}
		free(v->val);
	}
	
	v->val = NULL;
	v->cap = 0;
	v->argc = 0;
}




/*
 * vec_find
 *
 * Линейный поиск элемента в Vector.
 *
 * Сравнение выполняется через функцию cmp (callback),
 * что позволяет работать с любыми типами данных.
 *
 * Возврат:
 *   >= 0 — индекс найденного элемента
 *   -1   — элемент не найден или ошибка входных параметров
 *
 * Память:
 *   не выделяет и не освобождает память
 *   не изменяет содержимое Vector
 */
 
ssize_t vec_find(const Vector *v, const void *key, int (*cmp)(const void*, const void*))
{
	if (!v || !v->val || !key || !cmp){
		errno = EINVAL;
		return -1;
	}
	
	for (size_t i = 0; i < v->argc; ++i){
		if (cmp(v->val[i], key) == 0)
			return i;
	}
	return -1;
}




/*
* функция возвращает элемент по индексу
*/

void *vec_get(const Vector *v, size_t idx)
{
	if (!v || !v->val){
		errno = EINVAL;
		return NULL;
	}
	
	if (idx >= v->argc){
		errno = ERANGE;
		return NULL;
	}
	
	return v->val[idx];
}




void *vec_pop(Vector *v)
{
	if (!v || !v->val){
		errno = EINVAL;
		return NULL;
	}
	
	if (v->argc == 0)
		return NULL;
	
	void *item = v->val[v->argc - 1];
	
	v->argc--;
	
	v->val[v->argc] = NULL;
	
	return item;
}




/*
 * Удаляет элемент по индексу.
 *
 * Если fn_free != NULL:
 *     вызывает fn_free(v->val[idx])
 *
 * После удаления:
 *     все элементы справа сдвигаются влево
 *     argc уменьшается
 *     сохраняется NULL-терминатор
 *
 * Ошибки:
 *     EINVAL
 *     ERANGE
 */
 
 int vec_remove(Vector *v, size_t idx, void (*free_fn) (void*))
{
	if (!v){
		errno = EINVAL;
		return -1;
	}
	
	if (idx >= v->argc){
		errno = ERANGE;
		return -1;
	}
 	
	if (free_fn)
		free_fn(v->val[idx]);
	
	memmove(&v->val[idx], &v->val[idx + 1], (v->argc - idx) * sizeof(*v->val));
	
	v->argc--;
	v->val[v->argc] = NULL;
	
	return 0;
}