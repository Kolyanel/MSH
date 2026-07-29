#include "readline_internal.h"

/*
** ============================================================
** Finish accepted line
** ============================================================
*/

int	rl_finish_line(
		t_rl *rl)
{
	size_t	end_row;
	size_t	end_col;

	if (!rl || rl->fd < 0)
		return (-1);

	/*
	** Suggestion is visual-only and must never become part
	** of the accepted command.
	*/
	rl_clear_suggestion(rl);

	/*
	** Recalculate the final position after suggestion removal.
	*/
	rl_calc_layout(rl);

	end_row = rl->layout.end_row;
	end_col = rl->layout.end_col;

	/*
	** Move to the actual end of the accepted input.
	*/
	if (rl_move_cursor(
			rl,
			end_row,
			end_col) < 0)
		return (-1);

	/*
	** Always terminate the accepted readline line explicitly.
	**
	** CR returns to column zero.
	** LF advances to the next physical row.
	*/
	if (rl_write_all(
			rl->fd,
			"\r\n",
			2) < 0)
		return (-1);

	rl->terminal.draw_rows = 0;

	rl->terminal.cursor_row =
		end_row + 1;

	rl->terminal.cursor_col = 0;

	rl->terminal.initialized = 0;

	return (0);
}