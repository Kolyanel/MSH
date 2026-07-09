#include <stdlib.h>
#include <string.h>

#include "readline_internal.h"
#include "buf.h"
#include "utf8.h"


static void	rl_clear_suggestion(t_rl *rl)
{
	if (!rl)
		return;

	free(rl->suggestion);

	rl->suggestion = NULL;
	rl->sugg_len = 0;
}


void	rl_insert(t_rl *rl, const char *s, size_t len)
{
	size_t	i;

	if (!rl || !s || len == 0)
		return;

	rl_clear_suggestion(rl);

	for (i = 0; i < len; i++)
	{
		if (buf_insert(
				&rl->buf,
				rl->cursor,
				s[i]) < 0)
			return;

		rl->cursor++;
	}

	if (rl->cursor > rl->buf.len)
		rl->cursor = rl->buf.len;
}


void	rl_backspace(t_rl *rl)
{
	size_t	prev;

	if (!rl)
		return;

	if (rl->cursor == 0)
		return;

	rl_clear_suggestion(rl);

	prev = utf8_prev(
		rl->buf.data,
		rl->cursor);

	while (rl->cursor > prev)
	{
		rl->cursor--;

		if (buf_delete(
				&rl->buf,
				rl->cursor) < 0)
		{
			rl->cursor++;
			return;
		}
	}
}


void	rl_delete(t_rl *rl)
{
	size_t	next;

	if (!rl)
		return;

	if (rl->cursor >= rl->buf.len)
		return;

	rl_clear_suggestion(rl);

	next = utf8_next(
		rl->buf.data,
		rl->cursor);

	while (rl->cursor < next)
	{
		if (buf_delete(
				&rl->buf,
				rl->cursor) < 0)
			return;

		next--;
	}
}