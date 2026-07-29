#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "history.h"



/*
** ============================================================
** Initialize history
**
** ============================================================
*/

int	hist_init(
		t_hist *hist)
{
	if (!hist)
	{
		errno = EINVAL;
		return (-1);
	}


	memset(
		hist,
		0,
		sizeof(*hist));


	return (0);
}



/*
** ============================================================
** Free history
**
** ============================================================
*/

void	hist_free(
		t_hist *hist)
{
	size_t	i;


	if (!hist)
		return ;


	i = 0;

	while (i < HIST_MAX)
	{
		free(hist->lines[i]);
		hist->lines[i] = NULL;

		i++;
	}


	hist->size = 0;
	hist->head = 0;
	hist->index = 0;
}



/*
** ============================================================
** Push new history entry
**
** head points to next insertion position.
**
** ============================================================
*/

int	hist_push(
		t_hist *hist,
		const char *str)
{
	char	*copy;
	size_t	last;


	if (!hist || !str || !*str)
	{
		errno = EINVAL;
		return (-1);
	}



	/*
	** Ignore consecutive duplicates.
	*/

	if (hist->size > 0)
	{
		last = (hist->head + HIST_MAX - 1)
				% HIST_MAX;


		if (hist->lines[last]
			&& strcmp(
				hist->lines[last],
				str) == 0)
		{
			hist->index = hist->head;
			return (0);
		}
	}



	copy = strdup(str);

	if (!copy)
	{
		errno = ENOMEM;
		return (-1);
	}



	free(hist->lines[hist->head]);


	hist->lines[hist->head] = copy;



	hist->head =
		(hist->head + 1) % HIST_MAX;



	if (hist->size < HIST_MAX)
		hist->size++;



	/*
	** New input position.
	*/

	hist->index = hist->head;



	return (0);
}



/*
** ============================================================
** Save history
**
** ============================================================
*/

static int	hist_write_all(
		int fd,
		const char *buf,
		size_t len)
{
	size_t	off;
	ssize_t	ret;


	off = 0;


	while (off < len)
	{
		ret = write(
				fd,
				buf + off,
				len - off);


		if (ret < 0)
		{
			if (errno == EINTR)
				continue ;

			return (-1);
		}


		off += (size_t)ret;
	}


	return (0);
}



int	hist_save(
		t_hist *hist,
		const char *filename)
{
	int		fd;
	size_t	start;
	size_t	i;


	if (!hist || !filename)
	{
		errno = EINVAL;
		return (-1);
	}



	fd = open(
			filename,
			O_WRONLY | O_CREAT | O_TRUNC,
			0600);


	if (fd < 0)
		return (-1);



	if (hist->size == HIST_MAX)
		start = hist->head;
	else
		start = 0;



	i = 0;


	while (i < hist->size)
	{
		size_t	idx;


		idx = (start + i) % HIST_MAX;


		if (hist->lines[idx])
		{
			if (hist_write_all(
					fd,
					hist->lines[idx],
					strlen(hist->lines[idx])) < 0)
			{
				close(fd);
				return (-1);
			}


			if (hist_write_all(
					fd,
					"\n",
					1) < 0)
			{
				close(fd);
				return (-1);
			}
		}


		i++;
	}



	close(fd);


	return (0);
}



/*
** ============================================================
** Load history
**
** ============================================================
*/

int	hist_load(
		t_hist *hist,
		const char *filename)
{
	FILE	*fp;
	char	*line;
	size_t	cap;


	if (!hist || !filename)
	{
		errno = EINVAL;
		return (-1);
	}



	fp = fopen(
			filename,
			"r");


	if (!fp)
	{
		if (errno == ENOENT)
			return (0);

		return (-1);
	}



	line = NULL;
	cap = 0;



	while (getline(
			&line,
			&cap,
			fp) >= 0)
	{
		size_t	len;


		len = strlen(line);


		if (len > 0
			&& line[len - 1] == '\n')
		{
			line[len - 1] = '\0';
		}



		if (*line)
		{
			hist_push(
				hist,
				line);
		}
	}



	free(line);

	fclose(fp);


	return (0);
}