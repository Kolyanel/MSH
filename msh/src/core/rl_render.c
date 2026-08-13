#include "readline_internal.h"
#include "utf8.h"


/*
** ============================================================
** Render one editable character
** ============================================================
**
** Терминал имеет особое состояние после записи символа
** в последнюю колонку строки: pending wrap.
**
** Мы не оставляем терминал в этом состоянии.
** После заполнения строки явно переводим его на следующую
** физическую строку.
** ============================================================
*/

static int	rl_render_char(
				t_rl *rl,
				const char *s,
				size_t len,
				size_t width)
{
	size_t	cols;

	if (!rl || !s || len == 0)
		return (-1);

	cols = rl->terminal.cols;
	if (cols == 0)
		cols = RL_DEFAULT_TERM_COLS;

	/*
	** Широкий символ не должен пересекать границу строки.
	*/
	if (width > cols - rl->terminal.cursor_col)
	{
		if (rl_write_all(rl->fd, "\r\033[1B", 5) < 0)
			return (-1);

		rl->terminal.cursor_row++;
		rl->terminal.cursor_col = 0;
	}

	if (rl_write_all(rl->fd, s, len) < 0)
		return (-1);

	rl->terminal.cursor_col += width;

	/*
	** После записи последней позиции терминал находится
	** в состоянии pending wrap.
	**
	** Немедленно переводим его физически на следующую строку.
	*/
	if (rl->terminal.cursor_col >= cols)
	{
		if (rl_write_all(rl->fd, "\r\033[1B", 5) < 0)
			return (-1);

		rl->terminal.cursor_row++;
		rl->terminal.cursor_col = 0;
	}

	return (0);
}


/*
** ============================================================
** Render arbitrary readline text
** ============================================================
**
** Общий renderer используется и для editable line,
** и для autosuggestion.
**
** Это важно: любой текст, который реально записан
** в терминал, одновременно должен обновлять внутреннее
** состояние terminal.cursor_row / cursor_col.
** ============================================================
*/

static int	rl_render_text(
				t_rl *rl,
				const char *s,
				size_t total_len)
{
	size_t	pos;
	size_t	len;
	size_t	width;

	if (!rl || rl->fd < 0)
		return (-1);

	if (!s || total_len == 0)
		return (0);

	pos = 0;

	while (pos < total_len)
	{
		/*
		** ANSI escape sequence.
		**
		** Она не занимает экранных колонок.
		**
		** Здесь намеренно не изменяем cursor_row/cursor_col:
		** renderer ожидает обычные управляющие последовательности
		** цвета/оформления, а не последовательности движения
		** курсора.
		*/
		if ((unsigned char)s[pos] == 0x1B)
		{
			len = utf8_ansi_len(s + pos);

			if (len == 0 || len > total_len - pos)
				len = 1;

			if (rl_write_all(rl->fd, s + pos, len) < 0)
				return (-1);

			pos += len;
			continue;
		}

		/*
		** Перевод строки.
		*/
		if (s[pos] == '\n')
		{
			if (rl_write_all(rl->fd, "\r\033[1B", 5) < 0)
				return (-1);

			rl->terminal.cursor_row++;
			rl->terminal.cursor_col = 0;

			pos++;
			continue;
		}

		/*
		** Возврат каретки.
		*/
		if (s[pos] == '\r')
		{
			if (rl_write_all(rl->fd, "\r", 1) < 0)
				return (-1);

			rl->terminal.cursor_col = 0;

			pos++;
			continue;
		}

		/*
		** UTF-8 символ.
		*/
		len = utf8_char_len(s + pos);

		if (len == 0 || len > total_len - pos)
			len = 1;

		width = utf8_char_width(s + pos);

		if (width == 0)
			width = 1;

		if (rl_render_char(
					rl,
					s + pos,
					len,
					width) < 0)
			return (-1);

		pos += len;
	}

	return (0);
}


/*
** ============================================================
** Render editable line
** ============================================================
*/

int	rl_render_line(
				t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return (-1);

	if (!rl->line.buffer.data || rl->line.buffer.len == 0)
		return (0);

	return (rl_render_text(
				rl,
				rl->line.buffer.data,
				rl->line.buffer.len));
}


/*
** ============================================================
** Render autosuggestion
** ============================================================
**
** Раньше suggestion просто записывался через rl_write_all().
**
** Это было ошибкой:
**
**     физический терминал  -> cursor ушёл вперёд
**     readline state       -> cursor остался на прежнем месте
**
** Теперь suggestion проходит через тот же renderer, что и
** editable line. Поэтому cursor_row/cursor_col всегда отражают
** фактическую позицию терминала после отрисовки suggestion.
** ============================================================
*/

int	rl_render_suggestion(
				t_rl *rl)
{
	if (!rl || rl->fd < 0)
		return (-1);

	if (!rl->suggestion.text
			|| rl->suggestion.bytes == 0)
		return (0);

	return (rl_render_text(
				rl,
				rl->suggestion.text,
				rl->suggestion.bytes));
}