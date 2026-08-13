#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "readline_internal.h"


static char	*rl_take_line(t_rl *rl)
{
	char	*result;

	if (!rl)
		return (NULL);

	result = malloc(
		rl->line.buffer.len + 1);

	if (!result)
		return (NULL);

	if (rl->line.buffer.len != 0)
	{
		memcpy(
			result,
			rl->line.buffer.data,
			rl->line.buffer.len);
	}

	result[rl->line.buffer.len] = '\0';

	return (result);
}


void	rl_destroy(t_rl *rl)
{
	if (!rl)
		return;

	rl_clear_suggestion(rl);

	buf_free(
		&rl->line.buffer);

	buf_free(
		&rl->history.saved_line);

	memset(
		rl,
		0,
		sizeof(*rl));
}


char	*readline_fd(
	int fd,
	const char *prompt,
	t_hist *hist)
{
	t_rl		rl;
	t_rl_event	ev;
	char		*result;
	int		saved_errno;

	(void)prompt;

	if (rl_init(&rl, fd) < 0)
		return (NULL);

	rl.history.hist = hist;

	if (hist)
		hist->index = hist->head;

	/*
	** Get terminal geometry before establishing origin.
	*/
	if (rl_get_terminal_size(&rl) < 0)
	{
		rl_destroy(&rl);
		return (NULL);
	}

	/*
	** Enter raw mode so the terminal can answer DSR
	** and readline can receive the response directly.
	*/
	if (rl_enable_raw(&rl) < 0)
	{
		rl_destroy(&rl);
		return (NULL);
	}

	/*
	** IMPORTANT:
	**
	** Do NOT calculate origin from prompt width.
	**
	** The prompt may have wrapped. The terminal already knows
	** the exact physical cursor position.
	*/
	if (rl_set_origin_from_terminal(&rl) < 0)
	{
		saved_errno = errno;

		rl_restore(&rl);
		rl_destroy(&rl);

		errno = saved_errno;
		return (NULL);
	}

	rl.terminal.draw_start_row =
		rl.terminal.origin_row;

	rl.terminal.draw_start_col =
		rl.terminal.origin_col;

	rl.terminal.draw_end_row =
		rl.terminal.origin_row;

	rl.terminal.draw_end_col =
		rl.terminal.origin_col;

	rl.terminal.draw_rows = 0;
	rl.terminal.initialized = 0;

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

		rl_process_event(
			&rl,
			&ev);
	}

	if (!rl.accepted)
	{
		saved_errno = errno;

		rl_restore(&rl);
		rl_destroy(&rl);

		errno = saved_errno;
		return (NULL);
	}

	if (rl.dirty)
		rl_redraw(&rl);

	rl_restore(&rl);

	rl_finish_line(&rl);

	result = rl_take_line(&rl);

	if (!result)
	{
		rl_destroy(&rl);
		return (NULL);
	}

	rl_destroy(&rl);

	return (result);
}