#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

#include "readline.h"
#include "readline_internal.h"
#include "buf.h"
#include "io.h"
#include "utf8.h"


char	*read_line_raw(int fd, t_hist *hist, char *prompt)
{
	t_rl			rl;
	struct termios	raw;
	int				ret;


	memset(&rl, 0, sizeof(rl));

	rl.fd = fd;
	rl.hist = hist;


	if (prompt)
	{
		rl.prompt = strdup(prompt);

		if (!rl.prompt)
			return (NULL);

		rl.prompt_len = utf8_display_len(prompt);
	}


	if (buf_init(&rl.buf) < 0)
	{
		free(rl.prompt);
		return (NULL);
	}


	if (tcgetattr(fd, &rl.old_t) < 0)
	{
		buf_free(&rl.buf);
		free(rl.prompt);
		return (NULL);
	}


	raw = rl.old_t;


	raw.c_lflag &= ~(ICANON | ECHO);

	raw.c_lflag |= ISIG;

	raw.c_iflag &= ~(IXON);


	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;


	if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
	{
		buf_free(&rl.buf);
		free(rl.prompt);
		return (NULL);
	}


	rl_redraw(&rl);


	while (1)
	{
		ret = rl_read_key(&rl);

		if (ret == -1)
		{
			rl_restore(&rl);

			buf_free(&rl.buf);
			free(rl.prompt);

			return (NULL);
		}


		if (ret == '\n')
			break;


		rl_redraw(&rl);
	}


	if (rl.prev_rows > 1)
	{
		printf_fd(
			fd,
			"\033[%zuB",
			rl.prev_rows - 1);
	}


	puts_fd(
		fd,
		"\r\n",
		2);


	rl_restore(&rl);


	free(rl.prompt);


	if (rl.buf.len == 0)
	{
		buf_free(&rl.buf);
		return (strdup(""));
	}


	return (buf_finalize(&rl.buf));
}


void	rl_restore(t_rl *rl)
{
	if (!rl)
		return;

	tcsetattr(
		rl->fd,
		TCSAFLUSH,
		&rl->old_t);
}