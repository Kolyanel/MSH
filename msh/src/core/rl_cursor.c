#include "readline_internal.h"
#include "utf8.h"

/*
** ============================================================
** Влево на один UTF-8 символ
** ============================================================
*/

void	rl_cursor_left(
	t_rl *rl)
{
	const char	*start;
	const char	*pos;

	if (!rl || rl->line.cursor == 0)
		return;

	start = rl->line.buffer.data;
	pos = start + rl->line.cursor;

	rl->line.cursor = (size_t)(
		utf8_prev(start, pos) - start);

	rl_clear_suggestion(rl);
	rl->dirty = 1;
}

/*
** ============================================================
** Вправо на один UTF-8 символ
** ============================================================
*/

void	rl_cursor_right(
	t_rl *rl)
{
	const char	*start;
	const char	*pos;
	const char	*next;

	if (!rl)
		return;

	if (rl->line.cursor >= rl->line.buffer.len)
		return;

	start = rl->line.buffer.data;
	pos = start + rl->line.cursor;

	next = utf8_next(pos);

	if (next <= pos)
		return;

	if (next > start + rl->line.buffer.len)
		next = start + rl->line.buffer.len;

	rl->line.cursor = (size_t)(next - start);

	rl_clear_suggestion(rl);
	rl->dirty = 1;
}

/*
** ============================================================
** В начало строки
** ============================================================
*/

void	rl_cursor_home(
	t_rl *rl)
{
	if (!rl)
		return;

	rl->line.cursor = 0;

	rl_clear_suggestion(rl);
	rl->dirty = 1;
}

/*
** ============================================================
** В конец строки
** ============================================================
*/

void	rl_cursor_end(
	t_rl *rl)
{
	if (!rl)
		return;

	rl->line.cursor = rl->line.buffer.len;

	rl_clear_suggestion(rl);
	rl->dirty = 1;
}