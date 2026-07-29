#include <errno.h>
#include <string.h>

#include "readline_internal.h"
#include "utf8.h"

/*
** ============================================================
** Insert byte range at cursor
** ============================================================
*/

int	rl_insert(
		t_rl *rl,
		const char *s,
		size_t len)
{
	size_t		tail_len;
	size_t		pos;
	size_t		remaining;
	t_utf8_char	ch;

	if (!rl || (!s && len != 0))
	{
		errno = EINVAL;
		return (-1);
	}

	if (len == 0)
		return (0);

	if (rl->line.cursor > rl->line.buffer.len)
		rl->line.cursor = rl->line.buffer.len;

	/*
	** Проверяем UTF-8 до изменения буфера.
	*/
	pos = 0;

	while (pos < len)
	{
		remaining = len - pos;

		if (utf8_decode(
				s + pos,
				remaining,
				&ch) < 0)
		{
			errno = EILSEQ;
			return (-1);
		}

		if (ch.bytes == 0
			|| ch.bytes > remaining)
		{
			errno = EILSEQ;
			return (-1);
		}

		pos += ch.bytes;
	}

	/*
	** Курсор должен находиться на UTF-8 boundary.
	*/
	if (rl->line.buffer.data)
	{
		rl->line.cursor = utf8_align_boundary(
			rl->line.buffer.data,
			rl->line.cursor);
	}

	tail_len =
		rl->line.buffer.len
		- rl->line.cursor;

	if (buf_reserve(
			&rl->line.buffer,
			rl->line.buffer.len + len) < 0)
		return (-1);

	memmove(
		rl->line.buffer.data
			+ rl->line.cursor
			+ len,
		rl->line.buffer.data
			+ rl->line.cursor,
		tail_len);

	memcpy(
		rl->line.buffer.data
			+ rl->line.cursor,
		s,
		len);

	rl->line.buffer.len += len;
	rl->line.cursor += len;

	rl_clear_suggestion(rl);

	rl->dirty = 1;

	return (0);
}

/*
** ============================================================
** Delete byte range
** ============================================================
*/

int	rl_delete_range(
		t_rl *rl,
		size_t start,
		size_t end)
{
	size_t	tail_len;

	if (!rl)
	{
		errno = EINVAL;
		return (-1);
	}

	if (start > end)
	{
		errno = EINVAL;
		return (-1);
	}

	if (start > rl->line.buffer.len)
		start = rl->line.buffer.len;

	if (end > rl->line.buffer.len)
		end = rl->line.buffer.len;

	if (rl->line.buffer.data)
	{
		start = utf8_align_boundary(
			rl->line.buffer.data,
			start);

		end = utf8_align_boundary(
			rl->line.buffer.data,
			end);
	}

	if (start >= end)
		return (0);

	tail_len =
		rl->line.buffer.len - end;

	memmove(
		rl->line.buffer.data + start,
		rl->line.buffer.data + end,
		tail_len);

	rl->line.buffer.len -= end - start;

	rl->line.cursor = start;

	rl_clear_suggestion(rl);

	rl->dirty = 1;

	return (0);
}