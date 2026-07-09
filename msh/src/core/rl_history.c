#include <stdlib.h>
#include <string.h>

#include "readline_internal.h"
#include "buf.h"
#include "msh_debug.h"



static void	rl_clear_suggestion(t_rl *rl)
{
	if (!rl)
		return;

	free(rl->suggestion);
	rl->suggestion = NULL;
	rl->sugg_len = 0;
}



static int	rl_load_history_line(t_rl *rl, const char *line)
{
	if (!rl || !line)
		return (-1);


	buf_free(&rl->buf);


	if (buf_init(&rl->buf) < 0)
		return (-1);


	if (buf_append_span(&rl->buf, line, strlen(line)) < 0)
	{
		buf_free(&rl->buf);
		return (-1);
	}


	rl->cursor = rl->buf.len;


	rl_clear_suggestion(rl);


	rl_redraw(rl);

	return (0);
}



static char	*rl_save_current_line(t_rl *rl)
{
	if (!rl || !rl->buf.data)
		return (NULL);


	return (strdup(rl->buf.data));
}



void	rl_history_up(t_rl *rl)
{
	size_t	start;
	size_t	idx;


	if (!rl || !rl->hist)
		return;


	if (rl->hist->size == 0)
		return;


	if (rl->hist_idx == 0)
	{
		free(rl->saved_line);

		rl->saved_line = rl_save_current_line(rl);


		rl->hist_idx = rl->hist->size;
	}


	if (rl->hist_idx == 0)
		return;


	rl->hist_idx--;


	start = (rl->hist->size < HIST_MAX)
		? 0
		: rl->hist->head;


	idx = (start + rl->hist_idx) % HIST_MAX;


	if (!rl->hist->lines[idx])
		return;


	if (rl_load_history_line(rl, rl->hist->lines[idx]) < 0)
		DBG_READLINE("history up: load failed\n");
}



void	rl_history_down(t_rl *rl)
{
	size_t	start;
	size_t	idx;


	if (!rl || !rl->hist)
		return;


	if (rl->hist_idx == 0)
		return;


	rl->hist_idx++;


	if (rl->hist_idx >= rl->hist->size)
	{
		buf_free(&rl->buf);


		if (buf_init(&rl->buf) == 0 && rl->saved_line)
		{
			buf_append_span(
				&rl->buf,
				rl->saved_line,
				strlen(rl->saved_line));
		}


		rl->cursor = rl->buf.len;


		free(rl->saved_line);
		rl->saved_line = NULL;


		rl_clear_suggestion(rl);


		rl->hist_idx = 0;


		rl_redraw(rl);

		return;
	}


	start = (rl->hist->size < HIST_MAX)
		? 0
		: rl->hist->head;


	idx = (start + rl->hist_idx) % HIST_MAX;


	if (!rl->hist->lines[idx])
		return;


	if (rl_load_history_line(rl, rl->hist->lines[idx]) < 0)
		DBG_READLINE("history down: load failed\n");
}