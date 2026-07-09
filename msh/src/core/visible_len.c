#include <stddef.h>

#include "readline.h"


/*
** Пропуск ANSI CSI последовательности.
**
** Формат:
**
** ESC [ параметры команда
**
** Например:
**
** \033[31m
** \033[1;34m
** \033[K
**
*/
static size_t	skip_ansi_csi(const char *s, size_t i)
{
	i += 2;


	while (s[i])
	{
		/*
		** Последний байт CSI:
		** диапазон 0x40 - 0x7E
		*/
		if ((unsigned char)s[i] >= 0x40
			&& (unsigned char)s[i] <= 0x7E)
		{
			return (i + 1);
		}

		i++;
	}


	return (i);
}



size_t	visible_len(const char *str)
{
	size_t	i;
	size_t	len;


	if (!str)
		return (0);


	i = 0;
	len = 0;


	while (str[i])
	{
		/*
		** ANSI ESC
		*/
		if ((unsigned char)str[i] == 0x1B
			&& str[i + 1] == '[')
		{
			i = skip_ansi_csi(str, i);

			continue;
		}


		/*
		** Обычный видимый символ
		*/
		len++;
		i++;
	}


	return (len);
}