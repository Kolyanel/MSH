#include "readline_internal.h"

/*
** ============================================================
** Render editable line
** ============================================================
*/

int	rl_render_line(
				t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return (-1);

	if (!rl->line.buffer.data
		|| rl->line.buffer.len == 0)
		return (0);

	return (rl_write_all(
				rl->fd,
				rl->line.buffer.data,
				rl->line.buffer.len));
}

/*
** ============================================================
** Render suggestion
** ============================================================
*/

int	rl_render_suggestion(
				t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return (-1);

	if (!rl->suggestion.text
		|| rl->suggestion.bytes == 0)
		return (0);

	return (rl_write_all(
				rl->fd,
				rl->suggestion.text,
				rl->suggestion.bytes));
}