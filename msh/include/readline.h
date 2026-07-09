#ifndef READLINE_H
#define READLINE_H

#include <stddef.h>

#include "history.h"


/*
** Читает строку в интерактивном режиме.
**
** fd:
**     файловый дескриптор терминала.
**
** hist:
**     история команд.
**
** prompt:
**     строка приглашения.
**     Память принадлежит вызывающему коду.
**     readline только использует её.
**
** Возвращает:
**
**     char *  - введённая строка без '\n'.
**               Освобождается вызывающим кодом.
**
**     NULL    - EOF или ошибка.
*/
char	*read_line_raw(
	int		fd,
	t_hist	*hist,
	char	*prompt);



/*
** Возвращает длину видимого текста.
**
** Учитывает ANSI escape-последовательности:
**
**     "\033[31mtext\033[0m"
**
** будет считаться как:
**
**     text
*/
size_t	visible_len(const char *str);


#endif /* READLINE_H */