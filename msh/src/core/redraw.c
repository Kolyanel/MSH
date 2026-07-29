#include <errno.h>
#include <stdio.h>

#include "readline_internal.h"

/*
** ============================================================
** Restore logical cursor
** ============================================================
**
** Rendering always finishes at:
**
**     layout.end_row
**     layout.end_col
**
** but editing may be located anywhere inside the rendered
** buffer.
**
** Therefore we move from render end to logical cursor.
**
** All layout coordinates are relative to readline origin.
** ============================================================
*/

static int	rl_restore_cursor(
		t_rl *rl)
{
	char	buf[64];
	int		len;
	size_t	target_col;

	if (!rl)
	{
		errno = EINVAL;
		return (-1);
	}

	/*
	** Rendering finishes at layout.end_*.
	** Move vertically back to the logical cursor row.
	*/
	if (rl->layout.end_row > rl->layout.cursor_row)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuA",
			rl->layout.end_row
				- rl->layout.cursor_row);

		if (len < 0 || (size_t)len >= sizeof(buf))
		{
			errno = EOVERFLOW;
			return (-1);
		}

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return (-1);
	}

	/*
	** Physical column depends on the row.
	**
	** First readline row:
	**
	**     physical column = origin_col + cursor_col
	**
	** Subsequent rows:
	**
	**     physical column = cursor_col
	*/
	if (rl->layout.cursor_row == 0)
		target_col =
			rl->terminal.origin_col
			+ rl->layout.cursor_col;
	else
		target_col =
			rl->layout.cursor_col;

	/*
	** Return to physical column zero before positioning.
	*/
	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return (-1);

	if (target_col != 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			target_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
		{
			errno = EOVERFLOW;
			return (-1);
		}

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return (-1);
	}

	/*
	** Store logical readline-relative cursor position.
	*/
	rl->terminal.cursor_row =
		rl->layout.cursor_row;

	rl->terminal.cursor_col =
		rl->layout.cursor_col;

	return (0);
}

/*
** ============================================================
** Save render bounds
** ============================================================
*/

static void	rl_save_render_bounds(
		t_rl *rl,
		size_t rows)
{
	if (!rl)
		return;

	/*
	** Rendering starts at readline-relative (0, 0).
	**
	** origin_col is the physical terminal column occupied by
	** the already printed prompt. It is NOT part of the
	** relative draw coordinate system.
	*/
	rl->terminal.draw_start_row = 0;
	rl->terminal.draw_start_col = 0;

	/*
	** Rendering ends where layout says the complete line +
	** suggestion ended.
	**
	** These coordinates are relative to readline origin.
	*/
	rl->terminal.draw_end_row =
		rl->layout.end_row;

	rl->terminal.draw_end_col =
		rl->layout.end_col;

	rl->terminal.draw_rows = rows;
}

/*
** ============================================================
** Establish readline origin
** ============================================================
**
** The caller has already printed the prompt.
**
** After rl_clear_render(), the physical terminal cursor is
** positioned at the beginning of the readline input area.
**
** The tracked cursor state must therefore also become:
**
**     row = 0
**     col = 0
**
** This is the important invariant:
**
**     terminal.cursor_* == physical cursor relative to
**     readline origin.
**
** ============================================================
*/

static int	rl_position_at_origin(
		t_rl *rl)
{
	if (!rl)
	{
		errno = EINVAL;
		return (-1);
	}

	/*
	** On the first redraw there is no previous render, so the
	** physical cursor is still immediately after the prompt.
	**
	** We explicitly establish that position.
	*/
	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return (-1);

	if (rl->terminal.origin_col != 0)
	{
		char	buf[32];
		int		len;

		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			rl->terminal.origin_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
		{
			errno = EOVERFLOW;
			return (-1);
		}

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return (-1);
	}

	/*
	** Physical cursor is now at readline-relative (0, 0).
	*/
	rl->terminal.cursor_row = 0;
	rl->terminal.cursor_col = 0;

	return (0);
}

/*
** ============================================================
** Full redraw
** ============================================================
*/

void	rl_redraw(
		t_rl *rl)
{
	size_t	rows;

	if (!rl || rl->fd < 0)
		return;

	/*
	** Refresh terminal geometry.
	*/
	if (rl_get_terminal_size(rl) < 0)
	{
		if (rl->terminal.cols == 0)
			rl->terminal.cols =
				RL_DEFAULT_TERM_COLS;

		if (rl->terminal.rows == 0)
			rl->terminal.rows =
				RL_DEFAULT_TERM_ROWS;
	}

	/*
	** Remove the complete previous rendering.
	**
	** After this call:
	**
	**     physical cursor = readline origin
	**     cursor_row      = 0
	**     cursor_col      = 0
	**     draw_rows       = 0
	*/
	if (rl->terminal.draw_rows != 0)
		rl_clear_render(rl);

	/*
	** Establish the physical readline origin.
	**
	** This is required only as a physical positioning operation.
	** The helper also synchronizes terminal.cursor_*.
	*/
	if (rl_position_at_origin(rl) < 0)
		return;

	/*
	** Calculate the complete layout before writing anything.
	**
	** All layout coordinates are relative to readline origin.
	*/
	rl_calc_layout(rl);

	/*
	** Render the editable buffer.
	*/
	if (rl_render_line(rl) < 0)
		return;

	/*
	** Render suggestion immediately after the editable buffer.
	**
	** Suggestion is display-only.
	** It never changes line.cursor.
	*/
	if (rl_render_suggestion(rl) < 0)
		return;

	/*
	** Calculate physical rows occupied by:
	**
	**     line + suggestion
	*/
	rows = rl_calc_rows(rl);

	if (rows == 0)
		rows = 1;

	/*
	** Store the exact bounds of this render.
	*/
	rl_save_render_bounds(
		rl,
		rows);

	/*
	** Rendering currently ends at layout.end_*.
	**
	** Restore the actual logical editing cursor.
	*/
	if (rl_restore_cursor(rl) < 0)
		return;

	/*
	** Synchronize tracked cursor state.
	*/
	rl->terminal.cursor_row =
		rl->layout.cursor_row;

	rl->terminal.cursor_col =
		rl->layout.cursor_col;

	rl->terminal.initialized = 1;
	rl->dirty = 0;
}