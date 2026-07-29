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


int buf_reserve(
        t_buf *b,
        size_t need);


int buf_init(t_buf *b);

int buf_clear(t_buf *b);

int buf_append(
        t_buf *b,
        char c);

int buf_append_span(
        t_buf *b,
        const char *s,
        size_t n);

int buf_insert(
        t_buf *b,
        size_t pos,
        char c);

int buf_insert_span(
        t_buf *b,
        size_t pos,
        const char *s,
        size_t n);

int buf_delete(
        t_buf *b,
        size_t pos);

int buf_delete_range(
        t_buf *b,
        size_t pos,
        size_t n);

char *buf_finalize(
        t_buf *b);

void buf_free(
        t_buf *b);


#endif