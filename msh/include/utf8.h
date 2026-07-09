#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>

/*
** Возвращает длину UTF-8 символа
** по первому байту.
**
** Возможные значения:
**
**     1
**     2
**     3
**     4
*/
size_t	utf8_char_len(unsigned char c);

/*
** Возвращает количество отображаемых
** символов в строке.
**
** ANSI escape-последовательности
** не учитываются.
*/
size_t	utf8_display_len(const char *str);

/*
** Возвращает смещение следующего
** UTF-8 символа.
**
** pos:
**     смещение в байтах.
*/
size_t	utf8_next(const char *str, size_t pos);

/*
** Возвращает смещение предыдущего
** UTF-8 символа.
**
** pos:
**     смещение в байтах.
*/
size_t	utf8_prev(const char *str, size_t pos);

/*
** Возвращает отображаемую позицию
** курсора (в экранных колонках)
** для байтового смещения.
**
** Используется readline
** при вычислении положения курсора.
*/
size_t	utf8_display_pos(
	const char *str,
	size_t byte_pos);

#endif /* UTF8_H */