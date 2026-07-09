#ifndef BUF_H
#define BUF_H

#include <stddef.h>

#define BUF_INIT_CAP 8


typedef struct s_buf
{
	char *data;
	size_t len;
	size_t cap;
} t_buf;


/* инициализирует буфер с начальной емкостью */

int buf_init(t_buf *b);



/* добавляет один символ в буфер */

int buf_append(t_buf *b, char c);



/* добавляет блок памяти в буфер */

int buf_append_span(t_buf *b, const char *s, size_t n);



/* отдает владение внутренним буфером вызывающему */

char *buf_finalize(t_buf *b);



/* освобождает буфер и сбрасывает состояние */

void buf_free(t_buf *b);


int buf_insert(t_buf *b, size_t pos, char c);


int buf_delete(t_buf *b, size_t pos);


#endif  //  BUF_H