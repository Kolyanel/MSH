#ifndef BUF_H
#define BUF_H

#include <stddef.h>


#define BUF_INIT_CAP 8



typedef struct s_buf
{
    char    *data;
    size_t  len;
    size_t  cap;

} t_buf;



/*
** Инициализация динамического буфера.
*/
int buf_init(
        t_buf *b);



/*
** Добавление одного байта.
*/
int buf_append(
        t_buf *b,
        char c);



/*
** Добавление блока данных.
*/
int buf_append_span(
        t_buf *b,
        const char *s,
        size_t n);



/*
** Вставка одного байта
** в указанную позицию.
*/
int buf_insert(
        t_buf *b,
        size_t pos,
        char c);



/*
** Вставка блока данных
** в указанную позицию.
**
** Используется readline
** для UTF-8 символов.
*/
int buf_insert_span(
        t_buf *b,
        size_t pos,
        const char *s,
        size_t n);



/*
** Удаление одного байта.
*/
int buf_delete(
        t_buf *b,
        size_t pos);


/*
** Удаление диапазона байтов.
**
** pos:
**     начальная позиция.
**
** n:
**     количество удаляемых байтов.
*/
int buf_delete_range(
        t_buf *b,
        size_t pos,
        size_t n);



/*
** Передача владения
** внутренним буфером.
*/
char *buf_finalize(
        t_buf *b);



/*
** Освобождение памяти.
*/
void buf_free(
        t_buf *b);


#endif