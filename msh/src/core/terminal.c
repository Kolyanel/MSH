#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#include "readline_internal.h"



int rl_write_all(int fd, const char *buf, size_t len)
{
	size_t done;
	ssize_t n;

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




int rl_enable_raw(t_rl *rl)
{
	struct termios raw;

	if (!rl || rl->fd < 0)
	{
		errno = EINVAL;
		return (-1);
	}

	if (tcgetattr(rl->fd, &rl->old_term) < 0)
		return (-1);

	raw = rl->old_term;

	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= CS8;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(rl->fd, TCSAFLUSH, &raw) < 0)
		return (-1);

	rl->terminal.raw_enabled = 1;
	return (0);
}



void rl_restore(t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return;

	if (!rl->terminal.raw_enabled)
		return;

	(void)tcsetattr(rl->fd, TCSAFLUSH, &rl->old_term);

	rl->terminal.raw_enabled = 0;
	rl->terminal.initialized = 0;
}




int rl_get_terminal_size(t_rl *rl)
{
	struct winsize ws;

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
** Ask the terminal for its real cursor position.
**
** DSR:
**     ESC [ 6 n
**
** Terminal response:
**     ESC [ row ; col R
**
** Returned coordinates are zero-based.
*/
int rl_get_cursor_position(
	t_rl *rl,
	size_t *row,
	size_t *col)
{
	char response[64];
	size_t i;
	size_t r;
	size_t c;
	ssize_t n;

	if (!rl || rl->fd < 0 || !row || !col)
	{
		errno = EINVAL;
		return (-1);
	}

	if (!rl->terminal.raw_enabled)
	{
		errno = EINVAL;
		return (-1);
	}

	if (rl_write_all(rl->fd, "\033[6n", 4) < 0)
		return (-1);

	i = 0;

	while (i < sizeof(response) - 1)
	{
		n = read(rl->fd, response + i, 1);

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

		i++;

		if (response[i - 1] == 'R')
			break;
	}

	if (i == 0 || response[i - 1] != 'R')
	{
		errno = EPROTO;
		return (-1);
	}

	response[i] = '\0';

	if (sscanf(response, "\033[%zu;%zuR", &r, &c) != 2)
	{
		errno = EPROTO;
		return (-1);
	}

	if (r == 0 || c == 0)
	{
		errno = EPROTO;
		return (-1);
	}

	*row = r - 1;
	*col = c - 1;

	return (0);
}




/*
** Set the readline origin to the REAL terminal cursor position.
**
** No prompt arithmetic is performed here.
*/
int rl_set_origin_from_terminal(t_rl *rl)
{
	size_t row;
	size_t col;

	if (!rl)
	{
		errno = EINVAL;
		return (-1);
	}

	if (rl_get_cursor_position(rl, &row, &col) < 0)
		return (-1);

	rl->terminal.origin_row = row;
	rl->terminal.origin_col = col;

	rl->terminal.cursor_row = row;
	rl->terminal.cursor_col = col;

	rl->terminal.draw_start_row = row;
	rl->terminal.draw_start_col = col;

	rl->terminal.draw_end_row = row;
	rl->terminal.draw_end_col = col;

	rl->terminal.draw_rows = 0;

	return (0);
}



/*
** Convert readline-relative coordinates to absolute terminal
** coordinates.
**
** (0,0) means the first editable cell immediately after prompt.
*/
void    rl_abs_position(
        t_rl *rl,
        size_t row,
        size_t col,
        size_t *abs_row,
        size_t *abs_col)
{
        if (!rl || !abs_row || !abs_col)
                return;

        *abs_row = rl->terminal.origin_row + row;

        if (row == 0)
                *abs_col = rl->terminal.origin_col + col;
        else
                *abs_col = col;
}




/*
** Move to a readline-relative position.
*/
int rl_move_cursor(t_rl *rl, size_t row, size_t col)
{
	char buf[64];
	int len;
	size_t delta;
	size_t target_row;
	size_t target_col;

	if (!rl || rl->fd < 0)
	{
		errno = EINVAL;
		return (-1);
	}

	rl_abs_position(
		rl,
		row,
		col,
		&target_row,
		&target_col);

	if (target_row > rl->terminal.cursor_row)
	{
		delta = target_row - rl->terminal.cursor_row;

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

		if (rl_write_all(rl->fd, buf, (size_t)len) < 0)
			return (-1);
	}
	else if (target_row < rl->terminal.cursor_row)
	{
		delta = rl->terminal.cursor_row - target_row;

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

		if (rl_write_all(rl->fd, buf, (size_t)len) < 0)
			return (-1);
	}

	if (rl_write_all(rl->fd, "\r", 1) < 0)
		return (-1);

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

		if (rl_write_all(rl->fd, buf, (size_t)len) < 0)
			return (-1);
	}

	rl->terminal.cursor_row = target_row;
	rl->terminal.cursor_col = target_col;

	return (0);
}




/*
** ============================================================
** Clear the previously rendered readline area.
** ============================================================
**
** The first rendered row starts at:
**
**     draw_start_row / draw_start_col
**
** Every following physical row starts at column 0 because
** readline rendering explicitly resolves terminal wrapping.
**
** Example:
**
**     terminal cols = 29
**     start         = (11,20)
**
**     row 11 -> column 20
**     row 12 -> column 0
**     row 13 -> column 0
**
** Therefore we must NOT use draw_start_col on rows after
** the first one.
** ============================================================
*/

void rl_clear_render(t_rl *rl)
{
	char buf[64];
	int len;
	size_t row;
	size_t rows;
	size_t target_row;
	size_t target_col;

	if (!rl || rl->fd < 0)
		return;

	rows = rl->terminal.draw_rows;

	if (rows == 0)
		return;

	target_row =
		rl->terminal.draw_start_row;

	target_col =
		rl->terminal.draw_start_col;

	if (getenv("MSH_CURSOR_DEBUG"))
	{
		fprintf(
			stderr,
			"CLEAR_DEBUG: "
			"cursor=(%zu,%zu) "
			"start=(%zu,%zu) "
			"end=(%zu,%zu) "
			"rows=%zu\n",
			rl->terminal.cursor_row,
			rl->terminal.cursor_col,
			rl->terminal.draw_start_row,
			rl->terminal.draw_start_col,
			rl->terminal.draw_end_row,
			rl->terminal.draw_end_col,
			rows);
	}

	/*
        ** ========================================================
        ** Move to the first rendered row.
        ** ========================================================
        */

	if (rl->terminal.cursor_row > target_row)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuA",
			rl->terminal.cursor_row - target_row);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}
	else if (rl->terminal.cursor_row < target_row)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuB",
			target_row - rl->terminal.cursor_row);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
        ** First rendered row begins at draw_start_col.
        */

	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return;

	if (target_col != 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			target_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
        ** Clear the first row from the origin to the right edge.
        */

	if (rl_write_all(
			rl->fd,
			"\033[0K",
			4) < 0)
		return;

	/*
        ** ========================================================
        ** Clear following physical rows.
        ** ========================================================
        **
        ** IMPORTANT:
        **
        ** After a real terminal wrap the next row begins at
        ** column zero.
        **
        ** Therefore we explicitly move:
        **
        **     DOWN + CR
        **
        ** and DO NOT restore target_col here.
        */

	row = 1;

	while (row < rows)
	{
		if (rl_write_all(
				rl->fd,
				"\033[1B\r",
				5) < 0)
			return;

		if (rl_write_all(
				rl->fd,
				"\033[2K",
				4) < 0)
			return;

		row++;
	}

	/*
        ** ========================================================
        ** Return to the first rendered row.
        ** ========================================================
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
        ** Restore the readline origin on the first row.
        */

	if (rl_write_all(
			rl->fd,
			"\r",
			1) < 0)
		return;

	if (target_col != 0)
	{
		len = snprintf(
			buf,
			sizeof(buf),
			"\033[%zuC",
			target_col);

		if (len < 0 || (size_t)len >= sizeof(buf))
			return;

		if (rl_write_all(
				rl->fd,
				buf,
				(size_t)len) < 0)
			return;
	}

	/*
        ** The terminal is now physically positioned at the
        ** readline origin.
        */

	rl->terminal.draw_start_row =
		target_row;

	rl->terminal.draw_start_col =
		target_col;

	rl->terminal.draw_end_row = 0;
	rl->terminal.draw_end_col = 0;
	rl->terminal.draw_rows = 0;

	rl->terminal.cursor_row =
		target_row;

	rl->terminal.cursor_col =
		target_col;
}