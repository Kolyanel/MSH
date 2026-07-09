#include <stdlib.h>

#include "readline_internal.h"
#include "utf8.h"


static void	rl_clear_suggestion(t_rl *rl)
{
	if (!rl)
		return;

	free(rl->suggestion);

	rl->suggestion = NULL;
	rl->sugg_len = 0;
}


void	rl_cursor_left(t_rl *rl)
{
	if (!rl)
		return;

	if (rl->cursor > 0)
	{
		rl->cursor = utf8_prev(
			rl->buf.data,
			rl->cursor);
	}

	rl_clear_suggestion(rl);
}


void	rl_cursor_right(t_rl *rl)
{
	if (!rl)
		return;

	if (rl->cursor < rl->buf.len)
	{
		rl->cursor = utf8_next(
			rl->buf.data,
			rl->cursor);
	}

	rl_clear_suggestion(rl);
}


void	rl_cursor_home(t_rl *rl)
{
	if (!rl)
		return;

	rl->cursor = 0;

	rl_clear_suggestion(rl);
}


void	rl_cursor_end(t_rl *rl)
{
	if (!rl)
		return;

	rl->cursor = rl->buf.len;

	rl_clear_suggestion(rl);
}