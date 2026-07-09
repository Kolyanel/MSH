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



void	rl_suggest(t_rl *rl)
{
	size_t		start;
	size_t		idx;
	size_t		i;
	const char	*line;


	if (!rl)
		return;


	rl_clear_suggestion(rl);


	if (!rl->hist)
		return;


	if (rl->hist->size == 0)
		return;


	if (!rl->buf.data || rl->buf.len == 0)
		return;


	start = (rl->hist->size < HIST_MAX)
		? 0
		: rl->hist->head;


	i = rl->hist->size;


	while (i > 0)
	{
		idx = (start + i - 1) % HIST_MAX;

		line = rl->hist->lines[idx];


		if (line
			&& strncmp(
				line,
				rl->buf.data,
				rl->buf.len) == 0
			&& strlen(line) > rl->buf.len)
		{
			rl->suggestion = strdup(line);


			if (!rl->suggestion)
			{
				DBG_READLINE(
					"suggest: strdup failed\n");

				return;
			}


			rl->sugg_len =
				strlen(rl->suggestion);


			return;
		}


		i--;
	}
}



void	rl_accept_suggestion(t_rl *rl)
{
	if (!rl)
		return;


	if (!rl->suggestion)
		return;


	if (rl->cursor != rl->buf.len)
		return;



	buf_free(&rl->buf);


	if (buf_init(&rl->buf) < 0)
	{
		rl_clear_suggestion(rl);
		return;
	}


	if (buf_append_span(
			&rl->buf,
			rl->suggestion,
			rl->sugg_len) < 0)
	{
		buf_free(&rl->buf);
		buf_init(&rl->buf);

		rl_clear_suggestion(rl);

		return;
	}


	rl->cursor = rl->buf.len;


	rl_clear_suggestion(rl);
}