#include <stdlib.h>

#include "readline_internal.h"


static void	rl_clear_suggestion(t_rl *rl)
{
	if (!rl)
		return;

	free(rl->suggestion);

	rl->suggestion = NULL;
	rl->sugg_len = 0;
}



static int	utf8_cont(unsigned char c)
{
	return ((c & 0xC0) == 0x80);
}



static size_t	utf8_prev_pos(t_rl *rl, size_t pos)
{
	if (!rl || pos == 0)
		return (0);


	pos--;


	while (pos > 0
		&& utf8_cont((unsigned char)rl->buf.data[pos]))
	{
		pos--;
	}


	return (pos);
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
		{
			return;
		}


		rl->cursor++;
	}


	if (rl->cursor > rl->buf.len)
		rl->cursor = rl->buf.len;
}



void	rl_backspace(t_rl *rl)
{
	size_t	start;


	if (!rl)
		return;


	if (rl->cursor == 0)
		return;


	rl_clear_suggestion(rl);


	start = utf8_prev_pos(
		rl,
		rl->cursor);


	while (rl->cursor > start)
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


	if (rl->cursor > rl->buf.len)
		rl->cursor = rl->buf.len;
}



void	rl_delete(t_rl *rl)
{
	size_t	next;


	if (!rl)
		return;


	if (rl->cursor >= rl->buf.len)
		return;


	rl_clear_suggestion(rl);


	next = rl->cursor + 1;


	while (next < rl->buf.len
		&& utf8_cont((unsigned char)rl->buf.data[next]))
	{
		next++;
	}


	while (next > rl->cursor)
	{
		if (buf_delete(
				&rl->buf,
				rl->cursor) < 0)
		{
			return;
		}

		next--;
	}
}