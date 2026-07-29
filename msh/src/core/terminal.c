#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "readline_internal.h"

/*
** ============================================================
** Write all
** ============================================================
*/

int	rl_write_all(
		int fd,
		const char *buf,
		size_t len)
{
	size_t	done;
	ssize_t	n;

	if (fd < 0 || (!buf && len != 0))
	{
		errno = EINVAL;
		return (-1);
	}

	done = 0;
	while (done < len)
	{
		n = write(fd, buf + done, len - done);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			return (-1);
		}
		if (n == 0)
		{
			errno = EIO;
			return (-1);
		}
		done += (size_t)n;
	}

	return (0);
}

/*
** ============================================================
** Enable raw mode
** ============================================================
*/

int	rl_enable_raw(
		t_rl *rl)
{
	struct termios	raw;

	if (!rl || rl->fd < 0)
	{
		errno = EINVAL;
		return (-1);
	}

	if (tcgetattr(rl->fd, &rl->old_term) < 0)
		return (-1);

	raw = rl->old_term;

	raw.c_iflag &= ~(BRKINT
			| ICRNL
			| INPCK
			| ISTRIP
			| IXON);

	raw.c_oflag &= ~(OPOST);

	raw.c_cflag |= CS8;

	raw.c_lflag &= ~(ECHO
			| ICANON
			| IEXTEN
			| ISIG);

	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(rl->fd, TCSAFLUSH, &raw) < 0)
		return (-1);

	rl->terminal.raw_enabled = 1;

	return (0);
}

/*
** ============================================================
** Restore terminal
** ============================================================
*/

void	rl_restore(
		t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return;

	if (!rl->terminal.raw_enabled)
		return;

	(void)tcsetattr(
		rl->fd,
		TCSAFLUSH,
		&rl->old_term);

	rl->terminal.raw_enabled = 0;
	rl->terminal.initialized = 0;
}

/*
** ============================================================
** Terminal size
** ============================================================
*/

int	rl_get_terminal_size(
		t_rl *rl)
{
	struct winsize	ws;

	if (!rl || rl->fd < 0)
	{
		errno = EINVAL;
		return (-1);
	}

	if (ioctl(rl->fd, TIOCGWINSZ, &ws) < 0)
		return (-1);

	if (ws.ws_row != 0)
		rl->terminal.rows = (size_t)ws.ws_row;
	else
		rl->terminal.rows = RL_DEFAULT_TERM_ROWS;

	if (ws.ws_col != 0)
		rl->terminal.cols = (size_t)ws.ws_col;
	else
		rl->terminal.cols = RL_DEFAULT_TERM_COLS;

	return (0);
}

/*
** ============================================================
** Move cursor
** ============================================================
**
** All coordinates passed here are relative to the readline
** origin.
**
**     row = physical row relative to readline start
**     col = physical column relative to readline start
**
** The prompt occupies terminal columns before origin_col.
**
** IMPORTANT:
**
** We do not use CUP here.
**
** The terminal position is changed using relative movement
** from the currently tracked readline cursor.
**
** This keeps the terminal state independent from the shell's
** absolute row number.
** ============================================================
*/

int	rl_move_cursor(
		t_rl *rl,
		size_t row,
		size_t col)
{
	char	buf[64];
	int		len;
	size_t	delta;
	size_t	target_col;

	if (!rl || rl->fd < 0)
	{
		errno = EINVAL;
		return (-1);
	}

	/*
	** Vertical movement.
	*/
	if (row > rl->terminal.cursor_row)
	{
		delta = row - rl->terminal.cursor_row;

		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuB",
			delta);

		if (len < 0 || (size_t)len >= sizeof(buf))
		{
			errno = EOVERFLOW;
			return (-1);
		}

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return (-1);
	}
	else if (row < rl->terminal.cursor_row)
	{
		delta = rl->terminal.cursor_row - row;

		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuA",
			delta);

		if (len < 0 || (size_t)len >= sizeof(buf))
		{
			errno = EOVERFLOW;
			return (-1);
		}

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return (-1);
	}

	/*
	** Horizontal positioning.
	**
	** We first return to column zero, then move to the actual
	** terminal column occupied by the readline position.
	*/
	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return (-1);

	if (row == 0)
		target_col =
			rl->terminal.origin_col + col;
	else
		target_col = col;

	if (target_col != 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			target_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
		{
			errno = EOVERFLOW;
			return (-1);
		}

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return (-1);
	}

	rl->terminal.cursor_row = row;
	rl->terminal.cursor_col = col;

	return (0);
}

/*
** ============================================================
** Clear previous rendering
** ============================================================
**
** The previous readline rendering occupies:
**
**     draw_rows
**
** physical rows.
**
** The first readline row is special:
**
**     [ already printed prompt ][ readline rendering ]
**     ^                         ^
**     column 0                 origin_col
**
** Therefore the first row MUST NOT be cleared from column 0.
**
** First row:
**
**     move to origin_col
**     CSI 0 K
**
** Subsequent rows:
**
**     move to column 0
**     CSI 2 K
**
** This preserves the shell prompt while removing the old
** readline rendering.
**
** ============================================================
*/

void	rl_clear_render(
		t_rl *rl)
{
	char	buf[64];
	int		len;
	size_t	row;
	size_t	rows;

	if (!rl || rl->fd < 0)
		return;

	rows = rl->terminal.draw_rows;

	if (rows == 0)
		return;

	/*
	** Current cursor coordinates are relative to readline origin.
	**
	** Move vertically back to the first readline row.
	*/
	if (rl->terminal.cursor_row > 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuA",
			rl->terminal.cursor_row);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
	** ========================================================
	** First physical row
	** ========================================================
	**
	** The prompt occupies:
	**
	**     columns [0, origin_col)
	**
	** Do NOT clear those columns.
	**
	** Return to column zero, then move to readline origin.
	*/
	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return;

	if (rl->terminal.origin_col != 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			rl->terminal.origin_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
	** Clear only the readline part of the first row.
	**
	** CSI 0 K:
	**
	**     erase from cursor to end of line.
	**
	** The prompt before origin_col remains untouched.
	*/
	if (rl_write_all(
			rl->fd,
			"\033[0K",
			4) < 0)
		return;

	/*
	** ========================================================
	** Subsequent physical rows
	** ========================================================
	*/
	row = 1;
	while (row < rows)
	{
		/*
		** Move to the next physical row and column zero.
		*/
		if (rl_write_all(
				rl->fd,
				"\033[1B\r",
				5) < 0)
			return;

		/*
		** These rows contain only readline rendering.
		** Therefore the complete physical row can be cleared.
		*/
		if (rl_write_all(
				rl->fd,
				"\033[2K",
				4) < 0)
			return;

		row++;
	}

	/*
	** We are now at the beginning of the last cleared row.
	**
	** Return to the first readline row.
	*/
	if (rows > 1)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuA",
			rows - 1);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
	** Return to physical column zero.
	*/
	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return;

	/*
	** Move to readline origin.
	**
	** origin_col is NOT part of the relative coordinate system.
	** It is the physical width occupied by the already printed
	** prompt.
	*/
	if (rl->terminal.origin_col != 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			rl->terminal.origin_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
	** Previous rendering no longer exists.
	*/
	rl->terminal.draw_start_row = 0;
	rl->terminal.draw_start_col = 0;

	rl->terminal.draw_end_row = 0;
	rl->terminal.draw_end_col = 0;

	rl->terminal.draw_rows = 0;

	/*
	** Cursor is now physically at readline origin.
	**
	** These coordinates are relative to readline origin.
	*/
	rl->terminal.cursor_row = 0;
	rl->terminal.cursor_col = 0;
}