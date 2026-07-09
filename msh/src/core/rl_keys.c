#include <unistd.h>

#include "readline_internal.h"
#include "utf8.h"



static int	read_seq(int fd, char *seq, int max)
{
	int	i;


	i = 0;


	while (i < max - 1)
	{
		if (read(fd, &seq[i], 1) != 1)
			return (-1);


		if ((seq[i] >= 'A' && seq[i] <= 'Z')
			|| (seq[i] >= 'a' && seq[i] <= 'z')
			|| seq[i] == '~')
		{
			seq[i + 1] = '\0';
			return (i + 1);
		}

		i++;
	}

	return (-1);
}



static int	handle_escape(t_rl *rl)
{
	char	seq[8];
	int		n;


	n = read_seq(
		rl->fd,
		seq,
		sizeof(seq));


	if (n <= 0)
		return (0);


	if (seq[0] == '[')
	{
		if (n == 2)
		{
			if (seq[1] == 'A')
				rl_history_up(rl);

			else if (seq[1] == 'B')
				rl_history_down(rl);

			else if (seq[1] == 'C')
				rl_cursor_right(rl);

			else if (seq[1] == 'D')
				rl_cursor_left(rl);

			return (0);
		}


		if (seq[1] == '3'
			&& seq[2] == '~')
		{
			rl_delete(rl);
			return (0);
		}


		if (seq[1] == 'H')
		{
			rl_cursor_home(rl);
			return (0);
		}


		if (seq[1] == 'F')
		{
			rl_cursor_end(rl);
			return (0);
		}
	}


	if (seq[0] == 'O')
	{
		if (seq[1] == 'H')
			rl_cursor_home(rl);

		else if (seq[1] == 'F')
			rl_cursor_end(rl);
	}


	return (0);
}



int	rl_read_key(t_rl *rl)
{
	char	buf[5];
	char	c;
	size_t	len;
	size_t	i;


	if (!rl)
		return (-1);


	if (read(
			rl->fd,
			&c,
			1) != 1)
		return (-1);



	if ((unsigned char)c == 27)
		return (handle_escape(rl));



	if ((unsigned char)c == 4)
		return (-1);



	if (c == '\n'
		|| c == '\r')
		return ('\n');



	if ((unsigned char)c == 127
		|| c == '\b')
	{
		rl_backspace(rl);
		return (0);
	}



	if (c == '\t')
	{
		if (rl->suggestion)
			rl_accept_suggestion(rl);

		else
			rl_suggest(rl);

		return (0);
	}



	if ((unsigned char)c >= 32)
	{
		buf[0] = c;


		len = utf8_char_len(
			(unsigned char)c);


		for (i = 1; i < len; i++)
		{
			if (read(
					rl->fd,
					&buf[i],
					1) != 1)
			{
				return (-1);
			}
		}


		rl_insert(
			rl,
			buf,
			len);


		return (0);
	}


	return (0);
}