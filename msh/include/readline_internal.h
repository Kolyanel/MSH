#ifndef READLINE_INTERNAL_H
#define READLINE_INTERNAL_H

#include <stddef.h>
#include <termios.h>

#include "buf.h"
#include "history.h"


typedef struct s_rl
{
	int				fd;

	t_hist			*hist;


	t_buf			buf;


	/*
	** Позиция курсора в буфере.
	**
	** Хранит смещение в байтах UTF-8 строки.
	**
	** Перемещение возможно только
	** по границам UTF-8 символов.
	*/
	size_t			cursor;


	char			*prompt;


	/*
	** Видимая длина prompt
	** в экранных колонках.
	**
	** ANSI escape-последовательности
	** не учитываются.
	*/
	size_t			prompt_len;



	/*
	** Размер терминала.
	*/
	int				term_cols;
	int				term_rows;



	/*
	** Предыдущая отрисовка.
	**
	** Используется для удаления
	** старой строки при redraw.
	*/
	size_t			prev_rows;

	size_t			prev_cursor_row;

	size_t			prev_cursor_col;



	/*
	** История.
	*/
	size_t			hist_idx;

	char			*saved_line;



	/*
	** Автодополнение.
	*/
	char			*suggestion;

	size_t			sugg_len;



	/*
	** Старые настройки терминала.
	*/
	struct termios	old_t;


}	t_rl;



/*
** terminal
*/
void	rl_restore(t_rl *rl);



/*
** redraw
*/
void	rl_redraw(t_rl *rl);



/*
** keyboard
*/
int		rl_read_key(t_rl *rl);



/*
** editing
*/
void	rl_insert(
	t_rl		*rl,
	const char	*s,
	size_t		len);

void	rl_backspace(t_rl *rl);

void	rl_delete(t_rl *rl);



/*
** cursor
*/
void	rl_cursor_left(t_rl *rl);

void	rl_cursor_right(t_rl *rl);

void	rl_cursor_home(t_rl *rl);

void	rl_cursor_end(t_rl *rl);



/*
** history
*/
void	rl_history_up(t_rl *rl);

void	rl_history_down(t_rl *rl);



/*
** suggestion
*/
void	rl_suggest(t_rl *rl);

void	rl_accept_suggestion(t_rl *rl);


#endif /* READLINE_INTERNAL_H */