#ifndef VECTOR_H
#define VECTOR_H

#include <sys/types.h>
#include <stddef.h>

typedef struct Vector{
	void **val;
	size_t cap;
	size_t argc;
} Vector;



/*
* возвращает количество элементов в Vector
*/

static inline size_t vec_size(const Vector *v)
{
	return v ? v->argc : 0;
}


/* передача владения из src в dst
dst должен быть предварительно пустым */
static inline void vec_move(Vector *dst, Vector *src)
{
	if (!dst || !src)
		return;
	
	*dst = *src;
	
	src->val = NULL;
	src->argc = 0;
	src->cap = 0;
}



/*
 * vec_init - инициализация динамического массива указателей (Vector)
 *
 * Выделяет массив указателей в стиле argv с NULL-терминатором.
 *
 * Параметры:
 *   v   - указатель на Vector
 *   cap - начальная ёмкость (0 → 8)
 *
 * Возврат:
 *   0   - успех
 *  -1   - ошибка (errno = EINVAL | ENOMEM)
 *
 * После вызова:
 *   v->argc = 0
 *   v->val[0] = NULL
 *
 * Примечание:
 *   память освобождается через vec_free
 */
 
int vec_init(Vector *v, size_t cap);



/*
 * vec_push - добавляет элемент в Vector
 * v     - указатель на Vector
 * item     - указатель на данные
 * Возвращает 0 при успехе, -1 при ошибке (errno устанавливается).
 * Расширяет массив при необходимости, всегда поддерживает NULL-терминатор:
 *   v->val[v->argc] == NULL
 * v->argc увеличивается на 1.
 */
 
int vec_push(Vector *v, void *item);



/*
 * vec_free - освобождает память Vector
 * v         - указатель на Vector
 * free_fn - функция освобождения элементов (может быть NULL)
 *
 * После вызова:
 *   v->val  == NULL
 *   v->argc == 0
 *   v->cap  == 0
 */
 
void vec_free(Vector *v, void (*free_fn)(void*));



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
 
ssize_t vec_find(const Vector *v, const void *key, int (*cmp)(const void*, const void*));



/*
* функция возвращает элемент по индексу
*/

void *vec_get(const Vector *v, size_t idx);



/* возвращает последнее
значения из вектора*/

void *vec_pop(Vector *v);



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
 
 int vec_remove(Vector *v, size_t idx, void (*free_fn) (void*));

#endif //VECTOR_H