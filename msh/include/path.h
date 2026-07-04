#ifndef PATH_H
#define PATH_H

#include "vector.h"


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
 
int parser_path(Vector *path, const char *str);



// разбирает PATH на список имеющихся путей

int path_from_env(Vector *path, Vector *env);

#endif  //PATH_H