#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "readline_internal.h"



static int	rl_move_to_origin(t_rl *rl)
{
	if (!rl)
	{
		errno = EINVAL;
		return (-1);
	}

	return (rl_move_cursor(rl, 0, 0));
}

static int	rl_restore_cursor(t_rl *rl)
{
	if (!rl)
	{
		errno = EINVAL;
		return (-1);
	}

	return (rl_move_cursor(
		rl,
		rl->layout.cursor_row,
		rl->layout.cursor_col));
}



static void	rl_save_render_bounds(
		t_rl *rl,
		size_t rows)
{
	size_t	abs_row;
	size_t	abs_col;

	if (!rl)
		return;

	rl->terminal.draw_start_row =
		rl->terminal.origin_row;

	rl->terminal.draw_start_col =
		rl->terminal.origin_col;

	rl_abs_position(
		rl,
		rl->layout.end_row,
		rl->layout.end_col,
		&abs_row,
		&abs_col);

	rl->terminal.draw_end_row = abs_row;
	rl->terminal.draw_end_col = abs_col;
	rl->terminal.draw_rows = rows;
}




void	rl_redraw(t_rl *rl)
{
	size_t	rows;

	if (!rl || rl->fd < 0)
		return;

	if (rl_get_terminal_size(rl) < 0)
	{
		if (rl->terminal.cols == 0)
			rl->terminal.cols =
				RL_DEFAULT_TERM_COLS;

		if (rl->terminal.rows == 0)
			rl->terminal.rows =
				RL_DEFAULT_TERM_ROWS;
	}

	if (rl->terminal.draw_rows != 0)
		rl_clear_render(rl);

	if (rl_move_to_origin(rl) < 0)
		return;

	rl_calc_layout(rl);

	if (getenv("MSH_CURSOR_DEBUG"))
	{
		fprintf(
			stderr,
			"CURSOR_DEBUG: "
			"cursor=(%zu,%zu) "
			"end=(%zu,%zu) "
			"rows=%zu "
			"origin=(%zu,%zu)\n",
			rl->layout.cursor_row,
			rl->layout.cursor_col,
			rl->layout.end_row,
			rl->layout.end_col,
			rl->layout.rows,
			rl->terminal.origin_row,
			rl->terminal.origin_col);
	}

	if (rl_write_all(
			rl->fd,
			"\033[K",
			3) < 0)
		return;

	if (rl_render_line(rl) < 0)
		return;

	if (rl_render_suggestion(rl) < 0)
		return;

	rows = rl->layout.rows;

	if (rows == 0)
		rows = 1;

	rl_save_render_bounds(rl, rows);

	if (rl_restore_cursor(rl) < 0)
		return;

	rl->terminal.initialized = 1;
	rl->dirty = 0;
}