#include <errno.h>
#include <string.h>

#include "readline_internal.h"


int	rl_init(t_rl *rl, int fd)
{
	if (!rl || fd < 0)
	{
		errno = EINVAL;
		return (-1);
	}

	memset(
		rl,
		0,
		sizeof(*rl));

	rl->fd = fd;

	/*
	** Terminal state.
	**
	** Real origin is obtained later from the terminal
	** with DSR.
	*/
	rl->terminal.rows = 0;
	rl->terminal.cols = 0;

	rl->terminal.origin_row = 0;
	rl->terminal.origin_col = 0;

	rl->terminal.cursor_row = 0;
	rl->terminal.cursor_col = 0;

	rl->terminal.draw_start_row = 0;
	rl->terminal.draw_start_col = 0;

	rl->terminal.draw_end_row = 0;
	rl->terminal.draw_end_col = 0;

	rl->terminal.draw_rows = 0;

	rl->terminal.raw_enabled = 0;
	rl->terminal.initialized = 0;

	/*
	** Layout state.
	*/
	rl->layout.rows = 1;
	rl->layout.end_row = 0;
	rl->layout.end_col = 0;

	rl->layout.cursor_row = 0;
	rl->layout.cursor_col = 0;

	rl->layout.line_cols = 0;
	rl->layout.suggestion_cols = 0;

	/*
	** Editable line.
	*/
	if (buf_init(&rl->line.buffer) < 0)
	{
		errno = ENOMEM;
		return (-1);
	}

	rl->line.cursor = 0;

	/*
	** History saved line.
	*/
	if (buf_init(&rl->history.saved_line) < 0)
	{
		buf_free(
			&rl->line.buffer);

		errno = ENOMEM;
		return (-1);
	}

	rl->history.hist = NULL;
	rl->history.active = 0;

	/*
	** Suggestion state.
	*/
	rl->suggestion.text = NULL;
	rl->suggestion.bytes = 0;
	rl->suggestion.cols = 0;

	/*
	** Runtime state.
	*/
	rl->running = 1;
	rl->accepted = 0;
	rl->dirty = 1;

	return (0);
}