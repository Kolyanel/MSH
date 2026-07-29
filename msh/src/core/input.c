#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "readline_internal.h"
#include "utf8.h"

/*
** ============================================================
** Прочитать один байт
** ============================================================
*/

int	rl_read_byte(
	t_rl *rl,
	char *c)
{
	ssize_t	ret;

	if (!rl || !c)
	{
		errno = EINVAL;
		return (-1);
	}

	while (1)
	{
		ret = read(
			rl->fd,
			c,
			1);

		if (ret < 0)
		{
			if (errno == EINTR)
				continue;

			return (-1);
		}

		if (ret == 0)
			return (0);

		return (1);
	}
}

/*
** ============================================================
** Прочитать CSI-последовательность
** ============================================================
**
** Поддерживаются:
**
**     ESC [ A
**     ESC [ B
**     ESC [ C
**     ESC [ D
**     ESC [ H
**     ESC [ F
**
** и формы:
**
**     ESC [ 1 ~
**     ESC [ 3 ~
**     ESC [ 4 ~
**     ESC [ 7 ~
**     ESC [ 8 ~
**
** Параметры намеренно читаются полностью.
** ============================================================
*/

static int	rl_read_escape(
	t_rl *rl,
	t_rl_event *ev)
{
	char	c;
	char	params[16];
	size_t	len;

	if (!rl || !ev)
		return (-1);

	if (rl_read_byte(
			rl,
			&c) <= 0)
		return (-1);

	/*
	** После ESC должен идти '['.
	*/
	if (c != '[')
	{
		ev->key = RL_KEY_NONE;
		return (0);
	}

	len = 0;

	while (len < sizeof(params) - 1)
	{
		if (rl_read_byte(
				rl,
				&c) <= 0)
			return (-1);

		/*
		** Конечный символ CSI.
		*/
		if ((unsigned char)c >= 0x40
			&& (unsigned char)c <= 0x7E)
		{
			params[len] = '\0';

			switch (c)
			{
				case 'A':
					ev->key = RL_KEY_UP;
					return (0);

				case 'B':
					ev->key = RL_KEY_DOWN;
					return (0);

				case 'C':
					ev->key = RL_KEY_RIGHT;
					return (0);

				case 'D':
					ev->key = RL_KEY_LEFT;
					return (0);

				case 'H':
					ev->key = RL_KEY_HOME;
					return (0);

				case 'F':
					ev->key = RL_KEY_END;
					return (0);

				case '~':
					if (strcmp(params, "1") == 0
						|| strcmp(params, "7") == 0)
						ev->key = RL_KEY_HOME;
					else if (strcmp(params, "3") == 0)
						ev->key = RL_KEY_DELETE;
					else if (strcmp(params, "4") == 0
						|| strcmp(params, "8") == 0)
						ev->key = RL_KEY_END;
					else
						ev->key = RL_KEY_NONE;

					return (0);

				default:
					ev->key = RL_KEY_NONE;
					return (0);
			}
		}

		params[len++] = c;
	}

	ev->key = RL_KEY_NONE;
	return (0);
}

/*
** ============================================================
** Прочитать UTF-8 символ
** ============================================================
**
** Сначала определяем ожидаемую длину по первому байту.
** После чтения проверяем всю последовательность через
** utf8_decode().
**
** Некорректный UTF-8 не принимается как многобайтный символ.
** Первый байт возвращается как обычный байт.
** ============================================================
*/

static int	rl_read_utf8(
	t_rl *rl,
	t_rl_event *ev,
	char first)
{
	unsigned char	c;
	size_t			need;
	size_t			i;
	t_utf8_char		ch;

	if (!rl || !ev)
		return (-1);

	memset(
		ev,
		0,
		sizeof(*ev));

	ev->key = RL_KEY_CHAR;
	ev->data[0] = first;

	c = (unsigned char)first;

	if (c < 0x80)
		need = 1;
	else if ((c & 0xE0) == 0xC0)
		need = 2;
	else if ((c & 0xF0) == 0xE0)
		need = 3;
	else if ((c & 0xF8) == 0xF0)
		need = 4;
	else
		need = 1;

	i = 1;

	while (i < need)
	{
		if (rl_read_byte(
				rl,
				&ev->data[i]) <= 0)
			return (-1);

		i++;
	}

	/*
	** Проверяем полученную последовательность.
	*/
	if (utf8_decode(
			ev->data,
			need,
			&ch) < 0
		|| ch.bytes != need)
	{
		/*
		** Первый байт считаем отдельным байтом.
		*/
		ev->data[0] = first;
		ev->len = 1;
		return (0);
	}

	ev->len = need;
	return (0);
}

/*
** ============================================================
** Основной обработчик клавиши
** ============================================================
*/

int	rl_read_key(
	t_rl *rl,
	t_rl_event *ev)
{
	char	c;
	int		ret;

	if (!rl || !ev)
	{
		errno = EINVAL;
		return (-1);
	}

	memset(
		ev,
		0,
		sizeof(*ev));

	ret = rl_read_byte(
		rl,
		&c);

	if (ret == 0)
	{
		ev->key = RL_KEY_EOF;
		return (0);
	}

	if (ret < 0)
		return (-1);

	/*
	** ESC.
	*/
	if ((unsigned char)c == 0x1B)
		return (rl_read_escape(rl, ev));

	/*
	** Enter.
	*/
	if (c == '\n' || c == '\r')
	{
		ev->key = RL_KEY_ENTER;
		ev->len = 1;
		ev->data[0] = c;
		return (0);
	}

	/*
	** Backspace.
	*/
	if ((unsigned char)c == 127
		|| (unsigned char)c == 8)
	{
		ev->key = RL_KEY_BACKSPACE;
		ev->len = 1;
		ev->data[0] = c;
		return (0);
	}

	/*
	** Ctrl-D.
	*/
	if ((unsigned char)c == 4)
	{
		ev->key = RL_KEY_EOF;
		ev->len = 1;
		ev->data[0] = c;
		return (0);
	}

	/*
	** Ctrl-C.
	*/
	if ((unsigned char)c == 3)
	{
		ev->key = RL_KEY_INTERRUPT;
		ev->len = 1;
		ev->data[0] = c;
		return (0);
	}

	/*
	** Tab.
	*/
	if (c == '\t')
	{
		ev->key = RL_KEY_TAB;
		ev->len = 1;
		ev->data[0] = c;
		return (0);
	}

	/*
	** Обычный символ / UTF-8.
	*/
	return (rl_read_utf8(
		rl,
		ev,
		c));
}