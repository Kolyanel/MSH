#include <errno.h>
#include <string.h>

#include "readline_internal.h"

/*
** ============================================================
** Initialize readline
** ============================================================
*/

int	rl_init(
		t_rl *rl,
		int fd,
		size_t origin_row,
		size_t origin_col)
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
	** Readline origin.
	**
	** origin_row / origin_col describe the physical terminal
	** position where editable input begins.
	**
	** All logical readline coordinates are relative to this
	** origin.
	*/
	rl->terminal.origin_row = origin_row;
	rl->terminal.origin_col = origin_col;

	/*
	** The logical cursor initially points to the beginning
	** of the empty input line.
	**
	** IMPORTANT:
	**
	** cursor_col is relative to readline origin.
	**
	** Therefore an empty input line starts at:
	**
	**     row = 0
	**     col = 0
	**
	** The prompt width is stored separately in origin_col.
	*/
	rl->terminal.cursor_row = 0;
	rl->terminal.cursor_col = 0;

	/*
	** No rendering exists yet.
	*/
	rl->terminal.rows = 0;
	rl->terminal.cols = 0;

	rl->terminal.draw_start_row = 0;
	rl->terminal.draw_start_col = 0;

	rl->terminal.draw_end_row = 0;
	rl->terminal.draw_end_col = 0;

	rl->terminal.draw_rows = 0;

	rl->terminal.raw_enabled = 0;
	rl->terminal.initialized = 0;

	/*
	** Empty layout.
	**
	** Layout coordinates are relative to readline origin.
	**
	** The empty input therefore ends at:
	**
	**     row = 0
	**     col = 0
	**
	** The prompt is NOT part of the layout.
	*/
	rl->layout.rows = 1;

	rl->layout.end_row = 0;
	rl->layout.end_col = 0;

	rl->layout.cursor_row = 0;
	rl->layout.cursor_col = 0;

	rl->layout.line_cols = 0;
	rl->layout.suggestion_cols = 0;

	/*
	** Readline state.
	*/
	rl->running = 1;
	rl->accepted = 0;
	rl->dirty = 1;

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
		buf_free(&rl->line.buffer);
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

	return (0);
}