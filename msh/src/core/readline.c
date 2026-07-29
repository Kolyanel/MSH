#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "readline_internal.h"

/*
** ============================================================
** Take current line
** ============================================================
*/

static char	*rl_take_line(
		t_rl *rl)
{
	char	*result;

	if (!rl)
		return (NULL);

	result = malloc(rl->line.buffer.len + 1);
	if (!result)
		return (NULL);

	if (rl->line.buffer.len != 0)
		memcpy(
			result,
			rl->line.buffer.data,
			rl->line.buffer.len);

	result[rl->line.buffer.len] = '\0';

	return (result);
}

/*
** ============================================================
** Destroy readline
** ============================================================
*/

void	rl_destroy(
		t_rl *rl)
{
	if (!rl)
		return;

	rl_clear_suggestion(rl);

	buf_free(&rl->line.buffer);
	buf_free(&rl->history.saved_line);

	memset(rl, 0, sizeof(*rl));
}

/*
** ============================================================
** readline
** ============================================================
*/

char	*readline_fd(
		int fd,
		const char *prompt,
		t_hist *hist,
		size_t origin_row,
		size_t origin_col)
{
	t_rl		rl;
	t_rl_event	ev;
	char		*result;
	int			saved_errno;

	(void)prompt;
	(void)origin_row;

	if (rl_init(
			&rl,
			fd,
			origin_row,
			origin_col) < 0)
		return (NULL);

	rl.history.hist = hist;

	if (hist)
		hist->index = hist->head;

	if (rl_get_terminal_size(&rl) < 0)
	{
		rl_destroy(&rl);
		return (NULL);
	}

	if (rl_enable_raw(&rl) < 0)
	{
		rl_destroy(&rl);
		return (NULL);
	}

	/*
	** IMPORTANT:
	**
	** rl_init() has already converted the readline origin
	** to the internal coordinate system:
	**
	**     origin_row = 0
	**     origin_col = prompt visual width
	**
	** Do NOT overwrite these values with the arguments passed
	** to readline_fd().
	**
	** All layout coordinates are relative to this origin.
	*/

	rl.terminal.cursor_row = 0;
	rl.terminal.cursor_col = 0;

	rl.terminal.draw_start_row = 0;
	rl.terminal.draw_start_col =
		rl.terminal.origin_col;

	rl.terminal.draw_end_row = 0;
	rl.terminal.draw_end_col =
		rl.terminal.origin_col;

	rl.terminal.draw_rows = 0;
	rl.terminal.initialized = 1;

	rl.dirty = 1;

	while (rl.running)
	{
		if (rl.dirty)
			rl_redraw(&rl);

		if (rl_read_key(&rl, &ev) < 0)
		{
			saved_errno = errno;

			rl_restore(&rl);
			rl_destroy(&rl);

			errno = saved_errno;
			return (NULL);
		}

		rl_process_event(&rl, &ev);
	}

	/*
	** Ctrl-C / Ctrl-D.
	*/
	if (!rl.accepted)
	{
		saved_errno = errno;

		rl_restore(&rl);
		rl_destroy(&rl);

		errno = saved_errno;
		return (NULL);
	}

	/*
	** Enter may have invalidated the suggestion.
	*/
	if (rl.dirty)
		rl_redraw(&rl);

	/*
	** Restore terminal mode before returning.
	*/
	rl_restore(&rl);

	/*
	** Finish from the calculated END of the complete rendered
	** line, never from the logical editing cursor.
	*/
	if (rl_finish_line(&rl) < 0)
	{
		saved_errno = errno;

		rl_destroy(&rl);

		errno = saved_errno;
		return (NULL);
	}

	result = rl_take_line(&rl);

	if (!result)
	{
		rl_destroy(&rl);
		return (NULL);
	}

	rl_destroy(&rl);

	return (result);
}