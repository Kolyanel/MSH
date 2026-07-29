#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "readline_internal.h"
#include "utf8.h"



static int	is_executable(
		const char *path)
{
	struct stat	st;


	if (!path)
		return (0);


	if (stat(path, &st) < 0)
		return (0);


	if (!S_ISREG(st.st_mode))
		return (0);


	return (access(path, X_OK) == 0);
}



static int	match_exists(
		t_buf *matches,
		const char *name)
{
	char	*p;
	char	*end;
	size_t	len;


	if (!matches || !matches->data || !name)
		return (0);


	len = strlen(name);

	p = matches->data;


	while (*p)
	{
		end = strchr(p, '\n');

		if (!end)
			break;


		if ((size_t)(end - p) == len
			&& strncmp(p, name, len) == 0)
			return (1);


		p = end + 1;
	}


	return (0);
}



static void	search_dir(
		const char *dir,
		const char *prefix,
		t_buf *matches)
{
	DIR				*d;
	struct dirent	*ent;
	char			*path;
	size_t			path_len;
	size_t			prefix_len;
	size_t			name_len;


	if (!dir || !prefix || !matches)
		return;


	d = opendir(dir);

	if (!d)
		return;


	prefix_len = strlen(prefix);


	while ((ent = readdir(d)))
	{
		if (ent->d_name[0] == '.')
			continue;


		if (strncmp(
				ent->d_name,
				prefix,
				prefix_len) != 0)
			continue;


		if (match_exists(
				matches,
				ent->d_name))
			continue;


		name_len = strlen(ent->d_name);


		path_len =
			strlen(dir)
			+ name_len
			+ 2;


		path = malloc(path_len);

		if (!path)
			continue;


		snprintf(
			path,
			path_len,
			"%s/%s",
			dir,
			ent->d_name);



		if (is_executable(path))
		{
			if (buf_append_span(
					matches,
					ent->d_name,
					name_len) < 0)
			{
				free(path);
				break;
			}


			if (buf_append_span(
					matches,
					"\n",
					1) < 0)
			{
				free(path);
				break;
			}
		}


		free(path);
	}


	closedir(d);
}



static void	search_path(
		const char *prefix,
		t_buf *matches)
{
	char	*copy;
	char	*save;
	char	*dir;
	char	*env;


	env = getenv("PATH");

	if (!env)
		return;


	copy = strdup(env);

	if (!copy)
		return;


	save = NULL;


	dir = strtok_r(
			copy,
			":",
			&save);



	while (dir)
	{
		search_dir(
			dir,
			prefix,
			matches);


		dir = strtok_r(
				NULL,
				":",
				&save);
	}


	free(copy);
}



/*
** Find previous UTF-8 character boundary.
*/

static size_t	rl_prev_char(
		const char *s,
		size_t pos)
{
	size_t	i;


	if (!s || pos == 0)
		return (0);


	i = pos - 1;


	while (i > 0)
	{
		unsigned char	c;


		c = (unsigned char)s[i];


		if ((c & 0xC0) != 0x80)
			break;


		i--;
	}


	return (i);
}



/*
** Get current word byte range.
*/

static void	get_current_word(
		t_rl *rl,
		size_t *start,
		size_t *len)
{
	size_t	pos;
	size_t	begin;
	char	*data;


	if (!rl || !start || !len)
		return;


	*start = 0;
	*len = 0;


	data = rl->line.buffer.data;


	if (!data)
		return;


	pos = rl->line.cursor;


	if (pos > rl->line.buffer.len)
		pos = rl->line.buffer.len;


	pos = utf8_align_boundary(
			data,
			pos);



	begin = pos;


	while (begin > 0)
	{
		size_t	prev;


		prev = rl_prev_char(
				data,
				begin);



		if (data[prev] == ' '
			|| data[prev] == '\t'
			|| data[prev] == '|'
			|| data[prev] == ';'
			|| data[prev] == '&'
			|| data[prev] == '<'
			|| data[prev] == '>')
			break;


		begin = prev;
	}


	*start = begin;
	*len = pos - begin;
}



static size_t	build_array(
		t_buf *matches,
		char ***out)
{
	size_t	count;
	size_t	i;
	char	**arr;
	char	*p;
	char	*end;


	if (!matches || !matches->data || !out)
		return (0);


	*out = NULL;


	count = 0;

	p = matches->data;


	while (*p)
	{
		end = strchr(p, '\n');

		if (!end)
			break;


		count++;

		p = end + 1;
	}


	if (count == 0)
		return (0);



	arr = calloc(
			count + 1,
			sizeof(char *));


	if (!arr)
		return (0);



	p = matches->data;

	i = 0;


	while (*p && i < count)
	{
		end = strchr(p, '\n');


		arr[i] = strndup(
				p,
				end - p);



		if (!arr[i])
		{
			while (i)
				free(arr[--i]);


			free(arr);

			return (0);
		}


		i++;

		p = end + 1;
	}


	*out = arr;


	return (i);
}



static int	cmp_str(
		const void *a,
		const void *b)
{
	const char	*aa;
	const char	*bb;


	aa = *(const char **)a;
	bb = *(const char **)b;


	return (strcmp(aa, bb));
}



static size_t	common_prefix(
		char **arr,
		size_t count)
{
	size_t	len;
	size_t	i;


	if (!arr || count == 0)
		return (0);


	len = strlen(arr[0]);


	for (i = 1; i < count; i++)
	{
		size_t	j;


		j = 0;


		while (j < len
			&& arr[0][j]
			&& arr[i][j]
			&& arr[0][j] == arr[i][j])
			j++;


		len = j;
	}


	return (len);
}



static void	print_matches(
		t_rl *rl,
		char **arr,
		size_t count)
{
	size_t	i;


	if (!rl)
		return;


	rl_write_all(
		rl->fd,
		"\r\n",
		2);


	for (i = 0; i < count; i++)
	{
		rl_write_all(
			rl->fd,
			arr[i],
			strlen(arr[i]));


		rl_write_all(
			rl->fd,
			"  ",
			2);
	}


	rl_write_all(
		rl->fd,
		"\r\n",
		2);


	rl->dirty = 1;
}



void	rl_complete(
		t_rl *rl)
{
	t_buf	matches;
	char	*word;
	char	**arr;
	size_t	word_start;
	size_t	word_len;
	size_t	count;
	size_t	common;
	size_t	i;


	if (!rl)
		return;


	get_current_word(
		rl,
		&word_start,
		&word_len);


	if (word_len == 0)
		return;


	word = rl->line.buffer.data + word_start;


	if (buf_init(&matches) < 0)
		return;


	search_path(
		word,
		&matches);



	arr = NULL;


	count = build_array(
			&matches,
			&arr);


	if (count == 0)
	{
		buf_free(&matches);
		return;
	}



	qsort(
		arr,
		count,
		sizeof(char *),
		cmp_str);



	if (count == 1)
	{
		size_t	rest;


		rest = strlen(arr[0]);


		if (rest > word_len)
		{
			rl_insert(
				rl,
				arr[0] + word_len,
				rest - word_len);
		}


		rl_insert(
			rl,
			" ",
			1);
	}
	else
	{
		common = common_prefix(
				arr,
				count);


		if (common > word_len)
		{
			rl_insert(
				rl,
				arr[0] + word_len,
				common - word_len);
		}
		else
		{
			print_matches(
				rl,
				arr,
				count);
		}
	}



	for (i = 0; i < count; i++)
		free(arr[i]);


	free(arr);


	buf_free(&matches);


	rl_suggest(rl);


	rl->dirty = 1;
}