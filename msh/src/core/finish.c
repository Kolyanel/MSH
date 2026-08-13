#include "readline_internal.h"

/*
** ============================================================
** Finish accepted line
** ============================================================
*/

void	rl_finish_line(t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return;

	rl_clear_suggestion(rl);

	if (rl_write_all(rl->fd, "\r\n", 2) < 0)
		return;

	rl->terminal.draw_rows = 0;
	rl->terminal.cursor_col = 0;
	rl->terminal.initialized = 0;
}