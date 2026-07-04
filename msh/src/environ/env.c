#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "env.h"
#include "env_utils.h"
#include "io.h"
#include "memory.h"
#include "my_string.h"


// cmp для qsort: a и b — указатели на элементы массива (t_env**), сравнение по key

static int cmp_env(const void *a, const void *b)
{
	const t_env *e1 = *(const t_env **)a;
	const t_env *e2 = *(const t_env **)b;
	
	if ((!e1 || !e1->key) && (!e2 || !e2->key))
		return 0;
	
	if (!e1 || !e1->key)
		return -1;
	
	if (!e2 || !e2->key)
		return 1;
	
	return strcmp(e1->key, e2->key);
}



/* cmp env для поиска в Vector по ключу*/

static int cmp_env_key(const void *item, const void *key)
{
	const t_env *env = item;
	const char *name = key;
	
	if (!env || !env->key || !name)
		return -1;
	
	return strcmp(env->key, name);
}




// Выделяет и инициализирует структуру t_env.
// Возвращает указатель на новую структуру или NULL при ошибке (errno = ENOMEM).

t_env *env_init(void)
{
    t_env *e = malloc(sizeof(*e));
    	
    if (!e)
    	return NULL;
    	
    e->key = NULL;
    e->value = NULL;
    e->exported = 0;
    
    return e;
}




/*
 * env_from_envp
 *
 * Создаёт внутреннее окружение из envp:
 *   парсит "KEY=VALUE" и добавляет в env (Vector).
 *
 * Ошибки:
 *   ENOMEM → выход с ошибкой
 *   некорректные строки → пропускаются
 *
 * Возврат:
 *   0 / -1 (errno установлен)
 */

int env_from_envp(Vector *env, char **envp)
{
	if (!env || !envp){
		errno = EINVAL;
		return -1;
	}
	
	for (size_t i = 0; envp[i]; ++i){
		char *key = NULL;
		char *val = NULL;
		int mode = ASSIGN;
		
		if (parse_assignment(envp[i], &key, &val, &mode) < 0){
			
			free(key);
			free(val);
			if (errno == ENOMEM)
				return -1;
			
			continue;
		}
		
		t_env *var = env_set(env, key, val);
		free(key);
		free(val);
		
		if (!var)
			return -1;
		
		var->exported = 1;
	}
	return 0;
}




/*
 * env_find — найти переменную по имени
 *
 * Поведение:
 *   - Ищет переменную с совпадающим key (strcmp)
 *   - Возвращает указатель на t_env или NULL, если не найдено
 *
 * Ошибки:
 *   - env == NULL или key == NULL → NULL (errno = EINVAL)
 *
 * Важно:
 *   - Сравнивается только key
 *   - Возвращается оригинальный указатель (не копия)
 */
 
t_env *env_find(Vector *env, const char *key)
{
	if (!env || !key){
		errno = EINVAL;
		return NULL;
	}
	
	for (size_t i = 0; i < env->argc; ++i){
		
		t_env *var = env->val[i];
		
		if (var && var->key && strcmp(var->key, key) == 0)
			return var;
	}
	return NULL;
}




const char *env_get(Vector *env, const char *key)
{
	if (!env || !key){
		errno = EINVAL;
		return NULL;
	}
	
	t_env *e = env_find(env, key);
	
	if (!e || !e->value)
		return NULL;
	
	return e->value;
}




/*
 * Создаёт или обновляет переменную окружения.
 * Если ключ существует — обновляет значение.
 * Если нет — создаёт новую и добавляет в вектор.
 * Возвращает указатель на переменную или NULL при ошибке (EINVAL, ENOMEM).
 */
 
t_env *env_set(Vector *env, const char *key, const char *val)
{
	if (!env || !key){
		errno = EINVAL;
		return NULL;
	}
	
	if (!is_valid_identifier(key)){
		errno = EINVAL;
		return NULL;
	}
	
	t_env *var = env_find(env, key);
	
	if (!var){
		
		var = env_init();
		if (!var){
			errno = ENOMEM;
			return NULL;
		}
		
		var->key = strdup(key);
		if (!var->key){
			free(var);
			errno = ENOMEM;
			return NULL;
		}
		
		if (vec_push(env, var) < 0){
			free(var->key);
			free(var);
			errno = ENOMEM;
			return NULL;
		}
	}
	
	/*
 * val == NULL -> очищаем значение переменной (не удаляет key)
 */
	
	char *new_value = NULL;
	
	if (val){
		new_value = strdup(val);
		if (!new_value){
			errno = ENOMEM;
			return NULL;
		}
	}
	
	free(var->value);
	var->value = new_value;
	
	return var;
}




/*
 * Устанавливает или снимает флаг экспорта переменной окружения.
 * Работает не только с уже существующей переменной, создаёт новые.
 * Возвращает 0 при успехе, -1 при ошибке (EINVAL, ENOMEM).
 */
 
int env_export(Vector *env, const char *key, int mode)
{
	if (!env || !key || !*key){
		errno = EINVAL;
		return -1;
	}
		
	if (!is_valid_identifier(key)){
		errno = EINVAL;
		return -1;
	}
	
	t_env *var = env_find(env, key);
	
	if (!var){
		
		var = env_set(env, key, NULL);
		
		if (!var)
			return -1;
	}
	
	var->exported = (mode != 0);
	
	return 0;
}




// Печатает все экспортированные переменные в формате bash:
// "declare -x KEY=VALUE" или "declare -x KEY", если значения нет
// env — Vector с элементами типа t_env

void env_print_fd(Vector *env, int out_fd, int mode)
{
    if (!env || (mode != ENV_PRINT && mode != EXP_PRINT)){
    	return;
    }
    
	t_env **tmp = malloc(env->argc * sizeof(*tmp));
	
	if (!tmp)
		return;
	
	size_t j = 0;
	
	for (size_t i = 0; i < env->argc; ++i){
		t_env *v = env->val[i];
		if (v && v->key)
			tmp[j++] = v;
	}

    qsort(tmp, j, sizeof(*tmp), cmp_env);

    for (size_t i = 0; i < j; ++i){

        t_env *var = tmp[i];

        if (mode == ENV_PRINT){
            if (!var->exported)
            	continue;

            if (var->value)
                printf_fd(out_fd, "%s=%s\n", var->key, var->value);
        } else {
            if (var->value)
                printf_fd(out_fd,  "declare -x %s=\"%s\"\n", var->key, var->value);
            else
                printf_fd(out_fd, "declare -x %s\n", var->key);
        }
    }
    
    free(tmp);
}




// функция создания envp массива указателей на строки окружения

char **build_envp(Vector *env)
{
	if (!env){
		errno = EINVAL;
		return NULL;
	}
	
	size_t cnt = 0;
	
	for (size_t i = 0; i < env->argc; ++i){
		t_env *e = env->val[i];
		if (e && e->exported && e->key)
			cnt++;
	}
	
	char **arr = malloc((cnt + 1) * sizeof(*arr));
	
	if (!arr){
		errno = ENOMEM;
		return NULL;
	}
	
	size_t j = 0;
	
	for (size_t i = 0; i < env->argc; ++i){
		
		t_env *e = env->val[i];
		
		if (!e || !e->exported || !e->key)
			continue;
			
		arr[j] = cat_str_sep(e->key, e->value, '=', malloc_wrap);
		
		if (!arr[j]){
			free_arrp(arr, j);
			return NULL;
		}
		j++;
	}
	arr[j] = NULL;
	return arr;
}




// освобождение памяти после использования переменной окружения

void free_env(void *p)
{
	if (p){
		t_env *e = (t_env*) p;
		free(e->key);
		free(e->value);
		free(e);
	}
}




/*
 * Дописывает val к существующему значению переменной.
 *
 * Переменная должна существовать.
 * Новая переменная не создаётся.
 *
 * Ошибки:
 *   EINVAL - неверные аргументы
 *   ENOENT - переменная не найдена
 *   ENOMEM - ошибка выделения памяти
 */
 
t_env *env_append(Vector *env, const char *key, const char *val)
{
	if (!env || !key || !val){
		errno = EINVAL;
		return NULL;
	}
	
	t_env *e = env_find(env, key);
	
	if (!e){
		errno = ENOENT;
		return NULL;
	}
	
	if (!e->value){
		char *tmp = strdup(val);
		
		if (!tmp){
			errno = ENOMEM;
			return NULL;
		}
		
		e->value = tmp;
		
		return e;
	}
	
	char *res = str_join(e->value, val);
	
	if (!res)
		return NULL;
	
	free(e->value);
	e->value = res;
	
	return e;
}




/* удаление элемента t_env из Vector*/

int env_unset(Vector *env, const char *key)
{
	if (!env || !key){
		errno = EINVAL;
		return -1;
	}
	
	ssize_t idx = vec_find(env, key, cmp_env_key);
	
	if (idx < 0)
		return 0;
	
	return vec_remove(env, idx, free_env);
}




/*
 * Создаёт полную копию одной переменной окружения.
 *
 * Копируются:
 *   - key
 *   - value
 *   - exported
 *
 * Возвращает:
 *   указатель на новую t_env
 *
 * Ошибки:
 *   EINVAL - src == NULL
 *   ENOMEM - ошибка выделения памяти
 */
 
t_env *env_dup(const t_env *src)
{
	if (!src || !src->key){
		errno = EINVAL;
		return NULL;
	}
	
	t_env *tmp = env_init();
	
	if (!tmp){
		errno = ENOMEM;
		return NULL;
	}
	
	tmp->key = strdup(src->key);
	
	if (!tmp->key){
		errno = ENOMEM;
		free(tmp);
		return NULL;
	}
	
	if (src->value){
		tmp->value = strdup(src->value);
		
		if (!tmp->value){
			errno = ENOMEM;
			free(tmp->key);
			free(tmp);
			return NULL;
		}
	} else{
		tmp->value = NULL;
	}
	
	tmp->exported = src->exported;
	
	return tmp;
}



/*
 * Создаёт глубокую копию всего окружения.
 *
 * Для каждой переменной вызывается env_dup()
 * и копия добавляется в dst.
 *
 * Требования:
 *   dst должен быть предварительно инициализирован
 *   через vec_init().
 *
 * При ошибке:
 *   все уже созданные элементы dst освобождаются.
 *
 * Возвращает:
 *   0  - успех
 *  -1  - ошибка
 *
 * Ошибки:
 *   EINVAL - неверные аргументы
 *   ENOMEM - ошибка выделения памяти
 */
 
int env_clone(Vector *dst, const Vector *src)
{
	if (!dst || !src){
		errno = EINVAL;
		return -1;
	}
	
	for (size_t i = 0; i < src->argc; ++i){
		
		t_env *tmp = env_dup(src->val[i]);
		
		if (!tmp){
			vec_free(dst, free_env);
			return -1;
		}
		
		if (vec_push(dst, tmp) < 0){
			free_env(tmp);
			vec_free(dst, free_env);
			return -1;
		}
	}
	
	return 0;
}




/*
 * Выполняет присваивание переменной окружения.
 *
 * Режимы:
 *   ASSIGN -> env_set()
 *   APPEND -> env_append()
 *
 * Для APPEND:
 *   если переменная отсутствует,
 *   создаётся новая (bash-совместимое поведение).
 *
 * Возвращает:
 *   указатель на изменённую переменную
 *   или NULL при ошибке.
 *
 * Ошибки:
 *   EINVAL - неверные аргументы или режим
 *   ENOMEM - ошибка выделения памяти
 */
 
t_env *env_assign(Vector *env, const char *key, const char *val, int mode)
{
	if (!env || !key){
		errno = EINVAL;
		return NULL;
	}
	
	if (mode == ASSIGN)
		return env_set(env, key, val);
	
	if (mode == APPEND){
		t_env *e = env_append(env, key, val);
		
		if (!e && errno == ENOENT)
			return env_set(env, key, val);
		
		return e;
	}
	
	errno = EINVAL;
	return NULL;
}