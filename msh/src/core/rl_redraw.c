#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "readline_internal.h"
#include "io.h"
#include "utf8.h"


static void	rl_update_term_size(t_rl *rl)
{
	struct winsize	ws;


	if (!rl)
		return;


	if (ioctl(rl->fd, TIOCGWINSZ, &ws) < 0)
	{
		rl->term_cols = 80;
		rl->term_rows = 24;
		return;
	}


	rl->term_cols = ws.ws_col;
	rl->term_rows = ws.ws_row;


	if (rl->term_cols <= 0)
		rl->term_cols = 80;
}



static size_t	rl_total_display_len(t_rl *rl)
{
	if (!rl)
		return (0);


	return (
		rl->prompt_len
		+
		utf8_display_len(rl->buf.data)
	);
}



static size_t	rl_calc_rows(t_rl *rl)
{
	size_t	len;


	if (!rl)
		return (1);


	len = rl_total_display_len(rl);


	if (len == 0)
		return (1);


	return (
		(len - 1)
		/
		(size_t)rl->term_cols
		+
		1
	);
}



static void	rl_move_up(t_rl *rl, size_t rows)
{
	if (!rl || rows == 0)
		return;


	printf_fd(
		rl->fd,
		"\033[%zuA",
		rows);
}



static void	rl_clear_previous(t_rl *rl)
{
	size_t	i;


	if (!rl || rl->prev_rows == 0)
		return;


	rl_move_up(
		rl,
		rl->prev_rows - 1);


	for (i = 0; i < rl->prev_rows; i++)
	{
		puts_fd(
			rl->fd,
			"\r\033[K",
			4);


		if (i + 1 < rl->prev_rows)
		{
			puts_fd(
				rl->fd,
				"\033[1B",
				4);
		}
	}


	rl_move_up(
		rl,
		rl->prev_rows - 1);


	puts_fd(
		rl->fd,
		"\r",
		1);
}



static void	rl_calc_cursor(t_rl *rl)
{
	size_t	pos;


	if (!rl)
		return;


	pos =
		rl->prompt_len
		+
		utf8_display_pos(
			rl->buf.data,
			rl->cursor);


	rl->prev_cursor_row =
		pos
		/
		(size_t)rl->term_cols;


	rl->prev_cursor_col =
		pos
		%
		(size_t)rl->term_cols;
}



static void	rl_place_cursor_end(t_rl *rl)
{
	size_t	current;
	size_t	target;


	if (!rl)
		return;


	current =
		rl_total_display_len(rl);


	target =
		rl->prompt_len
		+
		utf8_display_pos(
			rl->buf.data,
			rl->cursor);


	if (current > target)
	{
		printf_fd(
			rl->fd,
			"\033[%zuD",
			current - target);
	}
}



void	rl_redraw(t_rl *rl)
{
	size_t	rows;


	if (!rl)
		return;


	rl_update_term_size(rl);


	if (rl->prev_rows)
		rl_clear_previous(rl);


	if (rl->prompt)
	{
		puts_fd(
			rl->fd,
			rl->prompt,
			strlen(rl->prompt));
	}


	if (rl->buf.len)
	{
		puts_fd(
			rl->fd,
			rl->buf.data,
			rl->buf.len);
	}


	rows = rl_calc_rows(rl);

	rl->prev_rows = rows;


	rl_calc_cursor(rl);


	rl_place_cursor_end(rl);
}