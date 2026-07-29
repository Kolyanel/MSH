#include "readline_internal.h"
#include "utf8.h"

/*
** ============================================================
** Удалить символ слева от курсора
** ============================================================
*/

void	rl_backspace(
	t_rl *rl)
{
	const char	*start;
	const char	*cursor;
	const char	*prev;
	size_t		start_pos;

	if (!rl || rl->line.cursor == 0)
		return;

	start = rl->line.buffer.data;
	cursor = start + rl->line.cursor;

	prev = utf8_prev(
		start,
		cursor);

	start_pos = (size_t)(prev - start);

	rl_delete_range(
		rl,
		start_pos,
		rl->line.cursor);
}

/*
** ============================================================
** Удалить символ справа от курсора
** ============================================================
*/

void	rl_delete(
	t_rl *rl)
{
	const char	*start;
	const char	*cursor;
	const char	*next;
	size_t		end_pos;

	if (!rl)
		return;

	if (rl->line.cursor >= rl->line.buffer.len)
		return;

	start = rl->line.buffer.data;
	cursor = start + rl->line.cursor;

	next = utf8_next(cursor);

	if (next <= cursor)
		return;

	if (next > start + rl->line.buffer.len)
		next = start + rl->line.buffer.len;

	end_pos = (size_t)(next - start);

	rl_delete_range(
		rl,
		rl->line.cursor,
		end_pos);
}