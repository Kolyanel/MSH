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
**
** Память prompt принадлежит вызывающему коду.
** readline только использует её.
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


#endif /* READLINE_H */