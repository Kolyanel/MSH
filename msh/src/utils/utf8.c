#include "utf8.h"


size_t	utf8_char_len(unsigned char c)
{
	if ((c & 0x80) == 0)
		return (1);

	if ((c & 0xE0) == 0xC0)
		return (2);

	if ((c & 0xF0) == 0xE0)
		return (3);

	if ((c & 0xF8) == 0xF0)
		return (4);

	return (1);
}


static size_t	skip_ansi(const char *s, size_t i)
{
	i += 2;

	while (s[i])
	{
		if ((unsigned char)s[i] >= 0x40
			&& (unsigned char)s[i] <= 0x7E)
			return (i + 1);

		i++;
	}

	return (i);
}


size_t	utf8_display_len(const char *str)
{
	size_t	i;
	size_t	len;


	if (!str)
		return (0);

	i = 0;
	len = 0;

	while (str[i])
	{
		if ((unsigned char)str[i] == 0x1B
			&& str[i + 1] == '[')
		{
			i = skip_ansi(str, i);
			continue;
		}

		i += utf8_char_len((unsigned char)str[i]);
		len++;
	}

	return (len);
}


size_t	utf8_next(const char *str, size_t pos)
{
	if (!str || !str[pos])
		return (pos);

	return (
		pos
		+
		utf8_char_len((unsigned char)str[pos])
	);
}


size_t	utf8_prev(const char *str, size_t pos)
{
	if (!str || pos == 0)
		return (0);

	pos--;

	while (pos > 0
		&& ((unsigned char)str[pos] & 0xC0) == 0x80)
	{
		pos--;
	}

	return (pos);
}


size_t	utf8_display_pos(
	const char *str,
	size_t byte_pos)
{
	size_t	i;
	size_t	pos;


	if (!str)
		return (0);

	i = 0;
	pos = 0;

	while (str[i] && i < byte_pos)
	{
		if ((unsigned char)str[i] == 0x1B
			&& str[i + 1] == '[')
		{
			i = skip_ansi(str, i);
			continue;
		}

		i += utf8_char_len((unsigned char)str[i]);
		pos++;
	}

	return (pos);
}