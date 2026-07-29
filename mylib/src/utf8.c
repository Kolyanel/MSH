#include <stddef.h>
#include <stdint.h>

#include "utf8.h"

/*
** ============================================================
** Проверить continuation byte
** ============================================================
**
** Каждый продолжительный байт UTF-8 имеет форму:
**
**     10xxxxxx
**
** ============================================================
*/

static int	utf8_is_cont(
	unsigned char c)
{
	return ((c & 0xC0) == 0x80);
}

/*
** ============================================================
** Определить экранную ширину Unicode-кодовой точки
** ============================================================
**
** Возвращает:
**
**   0 — управляющий или невидимый символ;
**   1 — обычный символ;
**   2 — широкий символ.
**
** Это не полноценная реализация Unicode wcwidth(),
** но для readline покрывает основные случаи:
**
** - ASCII;
** - управляющие символы;
** - combining marks;
** - CJK;
** - Hangul;
** - full-width формы;
** - emoji.
** ============================================================
*/

static size_t	utf8_calc_width(
	uint32_t cp)
{
	/*
	** C0 control characters.
	*/

	if (cp < 0x20)
		return (0);

	/*
	** DEL.
	*/

	if (cp == 0x7F)
		return (0);

	/*
	** C1 control characters.
	*/

	if (cp >= 0x80 && cp <= 0x9F)
		return (0);

	/*
	** Combining Diacritical Marks.
	*/

	if ((cp >= 0x0300 && cp <= 0x036F)
		|| (cp >= 0x0483 && cp <= 0x0489)
		|| (cp >= 0x0591 && cp <= 0x05BD)
		|| (cp >= 0x05BF && cp <= 0x05BF)
		|| (cp >= 0x05C1 && cp <= 0x05C2)
		|| (cp >= 0x05C4 && cp <= 0x05C5)
		|| (cp >= 0x0610 && cp <= 0x061A)
		|| (cp >= 0x064B && cp <= 0x065F)
		|| (cp >= 0x0670 && cp <= 0x0670)
		|| (cp >= 0x06D6 && cp <= 0x06DC)
		|| (cp >= 0x06DF && cp <= 0x06E4)
		|| (cp >= 0x06E7 && cp <= 0x06E8)
		|| (cp >= 0x06EA && cp <= 0x06ED)
		|| (cp >= 0x0711 && cp <= 0x0711)
		|| (cp >= 0x0730 && cp <= 0x074A)
		|| (cp >= 0x07A6 && cp <= 0x07B0)
		|| (cp >= 0x07EB && cp <= 0x07F3)
		|| (cp >= 0x0816 && cp <= 0x0819)
		|| (cp >= 0x081B && cp <= 0x0823)
		|| (cp >= 0x0825 && cp <= 0x0827)
		|| (cp >= 0x0829 && cp <= 0x082D)
		|| (cp >= 0x0859 && cp <= 0x085B)
		|| (cp >= 0x08D3 && cp <= 0x0902)
		|| (cp >= 0x093A && cp <= 0x093C)
		|| (cp >= 0x093E && cp <= 0x094F)
		|| (cp >= 0x0951 && cp <= 0x0957)
		|| (cp >= 0x0962 && cp <= 0x0963)
		|| (cp >= 0x1AB0 && cp <= 0x1AFF)
		|| (cp >= 0x1DC0 && cp <= 0x1DFF)
		|| (cp >= 0x20D0 && cp <= 0x20FF)
		|| (cp >= 0xFE00 && cp <= 0xFE0F)
		|| (cp >= 0xFE20 && cp <= 0xFE2F)
		|| (cp >= 0xE0100 && cp <= 0xE01EF))
	{
		return (0);
	}

	/*
	** Zero Width Joiner.
	*/

	if (cp == 0x200D)
		return (0);

	/*
	** Hangul Jamo.
	*/

	if ((cp >= 0x1100 && cp <= 0x115F)
		|| (cp >= 0x2329 && cp <= 0x232A)
		|| (cp >= 0x2E80 && cp <= 0x303E)
		|| (cp >= 0x3040 && cp <= 0xA4CF)
		|| (cp >= 0xAC00 && cp <= 0xD7A3)
		|| (cp >= 0xF900 && cp <= 0xFAFF)
		|| (cp >= 0xFE10 && cp <= 0xFE19)
		|| (cp >= 0xFE30 && cp <= 0xFE6F)
		|| (cp >= 0xFF00 && cp <= 0xFF60)
		|| (cp >= 0xFFE0 && cp <= 0xFFE6))
	{
		return (2);
	}

	/*
	** Emoji и связанные с ними символы.
	*/

	if ((cp >= 0x1F000 && cp <= 0x1FAFF)
		|| (cp >= 0x1FC00 && cp <= 0x1FFFD))
	{
		return (2);
	}

	/*
	** CJK Extension B и последующие диапазоны.
	*/

	if ((cp >= 0x20000 && cp <= 0x2FFFD)
		|| (cp >= 0x30000 && cp <= 0x3FFFD))
	{
		return (2);
	}

	/*
	** Остальные символы считаем одноклеточными.
	*/

	return (1);
}

/*
** ============================================================
** Декодировать один UTF-8 символ
** ============================================================
*/

int	utf8_decode(
	const char *s,
	size_t size,
	t_utf8_char *out)
{
	unsigned char	c;
	uint32_t		cp;

	if (!s || !out || size == 0)
		return (-1);

	/*
	** Начальные значения.
	**
	** При ошибке первый байт рассматривается как отдельный
	** байт шириной в одну экранную позицию.
	*/

	c = (unsigned char)s[0];

	out->codepoint = (uint32_t)c;
	out->bytes = 1;
	out->width = 1;

	/*
	** ASCII.
	*/

	if (c < 0x80)
	{
		out->width = utf8_calc_width(
			out->codepoint);
		return (0);
	}

	/*
	** --------------------------------------------------------
	** Двухбайтовая последовательность.
	** --------------------------------------------------------
	**
	** Первый байт должен быть C2..DF.
	**
	** C0 и C1 запрещены, поскольку они создают overlong
	** encoding ASCII.
	*/

	if (c >= 0xC2 && c <= 0xDF)
	{
		if (size < 2
			|| !utf8_is_cont(
				(unsigned char)s[1]))
			return (-1);

		cp = ((uint32_t)(c & 0x1F) << 6)
			| (uint32_t)(
				(unsigned char)s[1] & 0x3F);

		out->codepoint = cp;
		out->bytes = 2;
		out->width = utf8_calc_width(cp);

		return (0);
	}

	/*
	** --------------------------------------------------------
	** Трёхбайтовая последовательность.
	** --------------------------------------------------------
	*/

	if (c >= 0xE0 && c <= 0xEF)
	{
		unsigned char	c1;
		unsigned char	c2;

		if (size < 3)
			return (-1);

		c1 = (unsigned char)s[1];
		c2 = (unsigned char)s[2];

		if (!utf8_is_cont(c1)
			|| !utf8_is_cont(c2))
			return (-1);

		/*
		** E0 80..9F запрещён:
		** это overlong encoding.
		*/

		if (c == 0xE0 && c1 < 0xA0)
			return (-1);

		/*
		** ED A0..BF запрещён:
		** это UTF-16 surrogate range.
		*/

		if (c == 0xED && c1 >= 0xA0)
			return (-1);

		cp = ((uint32_t)(c & 0x0F) << 12)
			| ((uint32_t)(c1 & 0x3F) << 6)
			| (uint32_t)(c2 & 0x3F);

		out->codepoint = cp;
		out->bytes = 3;
		out->width = utf8_calc_width(cp);

		return (0);
	}

	/*
	** --------------------------------------------------------
	** Четырёхбайтовая последовательность.
	** --------------------------------------------------------
	*/

	if (c >= 0xF0 && c <= 0xF4)
	{
		unsigned char	c1;
		unsigned char	c2;
		unsigned char	c3;

		if (size < 4)
			return (-1);

		c1 = (unsigned char)s[1];
		c2 = (unsigned char)s[2];
		c3 = (unsigned char)s[3];

		if (!utf8_is_cont(c1)
			|| !utf8_is_cont(c2)
			|| !utf8_is_cont(c3))
			return (-1);

		/*
		** F0 80..8F запрещён:
		** overlong encoding.
		*/

		if (c == 0xF0 && c1 < 0x90)
			return (-1);

		/*
		** F4 90..BF запрещён:
		** кодовая точка выше U+10FFFF.
		*/

		if (c == 0xF4 && c1 > 0x8F)
			return (-1);

		cp = ((uint32_t)(c & 0x07) << 18)
			| ((uint32_t)(c1 & 0x3F) << 12)
			| ((uint32_t)(c2 & 0x3F) << 6)
			| (uint32_t)(c3 & 0x3F);

		out->codepoint = cp;
		out->bytes = 4;
		out->width = utf8_calc_width(cp);

		return (0);
	}

	/*
	** C0, C1, F5..FF и другие недопустимые первые байты.
	*/

	return (-1);
}

/*
** ============================================================
** Длина текущего UTF-8 символа
** ============================================================
**
** Здесь строка NUL-terminated, поэтому сначала определяем
** реальное количество доступных байт.
** ============================================================
*/

size_t	utf8_char_len(
	const char *s)
{
	size_t			size;
	t_utf8_char	ch;

	if (!s || *s == '\0')
		return (0);

	/*
	** Нам необходимо не более четырёх байт.
	*/

	size = 0;

	while (size < 4 && s[size] != '\0')
		size++;

	if (utf8_decode(
			s,
			size,
			&ch) < 0)
	{
		/*
		** Некорректный UTF-8 байт считаем отдельным байтом.
		*/

		return (1);
	}

	return (ch.bytes);
}

/*
** ============================================================
** Экранная ширина текущего UTF-8 символа
** ============================================================
*/

size_t	utf8_char_width(
	const char *s)
{
	size_t			size;
	t_utf8_char	ch;

	if (!s || *s == '\0')
		return (0);

	size = 0;

	while (size < 4 && s[size] != '\0')
		size++;

	if (utf8_decode(
			s,
			size,
			&ch) < 0)
	{
		/*
		** Невалидный байт отображаем как один символ.
		*/

		return (1);
	}

	return (ch.width);
}

/*
** ============================================================
** Длина ANSI CSI escape-последовательности
** ============================================================
**
** Поддерживаем последовательности вида:
**
**     ESC [ ... final
**
** где final находится в диапазоне:
**
**     0x40..0x7E
**
** Например:
**
**     ESC[31m
**     ESC[0m
**     ESC[2K
**     ESC[6n
**
** ESC без '[' считается отдельным управляющим байтом.
** ============================================================
*/

size_t	utf8_ansi_len(
	const char *s)
{
	size_t	i;

	if (!s || (unsigned char)s[0] != 0x1B)
		return (0);

	/*
	** ESC в конце строки.
	*/

	if (s[1] == '\0')
		return (1);

	/*
	** Это не CSI.
	*/

	if (s[1] != '[')
		return (1);

	i = 2;

	while (s[i] != '\0')
	{
		/*
		** Final byte CSI.
		*/

		if ((unsigned char)s[i] >= 0x40
			&& (unsigned char)s[i] <= 0x7E)
		{
			return (i + 1);
		}

		i++;
	}

	/*
	** Последовательность не завершена.
	** Возвращаем количество имеющихся байт.
	*/

	return (i);
}

/*
** ============================================================
** Экранная ширина первых bytes байт
** ============================================================
*/

size_t	utf8_width_n(
	const char *s,
	size_t bytes)
{
	size_t			width;
	size_t			len;
	size_t			available;
	t_utf8_char	ch;

	if (!s || bytes == 0)
		return (0);

	width = 0;

	while (*s != '\0' && bytes != 0)
	{
		/*
		** ANSI escape-последовательность.
		*/

		if ((unsigned char)*s == 0x1B)
		{
			len = utf8_ansi_len(s);

			if (len == 0)
				break;

			if (len > bytes)
				break;

			s += len;
			bytes -= len;
			continue;
		}

		/*
		** Определяем, сколько байт реально доступно.
		**
		** Не передаём utf8_decode() больше четырёх байт.
		*/

		available = bytes;

		if (available > 4)
			available = 4;

		/*
		** Если UTF-8 последовательность неполная или
		** некорректная, учитываем первый байт отдельно.
		*/

		if (utf8_decode(
			s,
			available,
			&ch) < 0)
		{
			width += 1;
			s++;
			bytes--;
			continue;
		}

		/*
		** Корректный символ.
		*/

		if (ch.bytes > bytes)
			break;

		width += ch.width;

		s += ch.bytes;
		bytes -= ch.bytes;
	}

	return (width);
}

/*
** ============================================================
** Экранная ширина NUL-terminated строки
** ============================================================
*/

size_t	utf8_width(
	const char *s)
{
	size_t	width;
	size_t	len;
	t_utf8_char	ch;

	if (!s)
		return (0);

	width = 0;

	while (*s != '\0')
	{
		/*
		** ANSI.
		*/

		if ((unsigned char)*s == 0x1B)
		{
			len = utf8_ansi_len(s);

			if (len == 0)
				break;

			s += len;
			continue;
		}

		/*
		** Для NUL-terminated строки сначала определяем,
		** сколько байт реально доступно.
		*/

		len = 0;

		while (len < 4 && s[len] != '\0')
			len++;

		if (utf8_decode(
			s,
			len,
			&ch) < 0)
		{
			width++;
			s++;
			continue;
		}

		width += ch.width;
		s += ch.bytes;
	}

	return (width);
}

/*
** ============================================================
** Количество UTF-8 символов
** ============================================================
*/

size_t	utf8_strlen(
	const char *s)
{
	size_t	count;
	size_t	len;

	if (!s)
		return (0);

	count = 0;

	while (*s != '\0')
	{
		/*
		** ANSI не является текстовым символом.
		*/

		if ((unsigned char)*s == 0x1B)
		{
			len = utf8_ansi_len(s);

			if (len == 0)
				break;

			s += len;
			continue;
		}

		len = utf8_char_len(s);

		if (len == 0)
			break;

		s += len;
		count++;
	}

	return (count);
}

/*
** ============================================================
** Перейти к следующему UTF-8 символу
** ============================================================
*/

const char	*utf8_next(
	const char *s)
{
	size_t	len;

	if (!s || *s == '\0')
		return (s);

	/*
	** ANSI рассматриваем как единое невидимое
	** управляющее выражение.
	*/

	if ((unsigned char)*s == 0x1B)
	{
		len = utf8_ansi_len(s);

		if (len == 0)
			return (s + 1);

		return (s + len);
	}

	len = utf8_char_len(s);

	if (len == 0)
		return (s);

	return (s + len);
}

/*
** ============================================================
** Перейти к предыдущему UTF-8 символу
** ============================================================
**
** Здесь предполагается, что s находится на границе символа.
**
** Для обычного UTF-8 достаточно пройти назад через
** continuation bytes.
**
** ANSI-последовательности специально не разбираются:
** readline использует эту функцию прежде всего для буфера
** редактируемого текста.
** ============================================================
*/

const char	*utf8_prev(
	const char *start,
	const char *s)
{
	const char	*p;

	if (!start || !s || s <= start)
		return (start);

	p = s - 1;

	while (p > start
		&& utf8_is_cont(
			(unsigned char)*p))
	{
		p--;
	}

	return (p);
}

/*
** ============================================================
** Количество байт для первых chars символов
** ============================================================
*/

size_t	utf8_bytes_for_chars(
	const char *s,
	size_t chars)
{
	size_t	bytes;
	size_t	len;

	if (!s || chars == 0)
		return (0);

	bytes = 0;

	while (*s != '\0' && chars != 0)
	{
		/*
		** ANSI не считается символом.
		*/

		if ((unsigned char)*s == 0x1B)
		{
			len = utf8_ansi_len(s);

			if (len == 0)
				break;

			bytes += len;
			s += len;
			continue;
		}

		len = utf8_char_len(s);

		if (len == 0)
			break;

		bytes += len;
		s += len;
		chars--;
	}

	return (bytes);
}

/*
** ============================================================
** Выравнять позицию на границу UTF-8 символа
** ============================================================
**
** Если pos указывает на continuation byte:
**
**     D0 [90]
**        ^
**
** функция вернёт позицию D0.
**
** Важно:
**
** pos должен находиться в пределах доступного буфера.
** ============================================================
*/

size_t	utf8_align_boundary(
	const char *s,
	size_t pos)
{
	if (!s)
		return (0);

	while (pos > 0
		&& utf8_is_cont(
			(unsigned char)s[pos]))
	{
		pos--;
	}

	return (pos);
}