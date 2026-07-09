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



static int	is_utf8_cont(unsigned char c)
{
	return ((c & 0xC0) == 0x80);
}



static size_t	utf8_prev(t_rl *rl, size_t pos)
{
	if (!rl || pos == 0)
		return (0);


	pos--;


	while (pos > 0
		&& is_utf8_cont(
			(unsigned char)rl->buf.data[pos]))
	{
		pos--;
	}


	return (pos);
}



static size_t	utf8_next(t_rl *rl, size_t pos)
{
	if (!rl || pos >= rl->buf.len)
		return (rl->buf.len);


	pos++;


	while (pos < rl->buf.len
		&& is_utf8_cont(
			(unsigned char)rl->buf.data[pos]))
	{
		pos++;
	}


	return (pos);
}



void	rl_cursor_left(t_rl *rl)
{
	if (!rl)
		return;


	rl->cursor = utf8_prev(
		rl,
		rl->cursor);


	rl_clear_suggestion(rl);
}



void	rl_cursor_right(t_rl *rl)
{
	if (!rl)
		return;


	rl->cursor = utf8_next(
		rl,
		rl->cursor);


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