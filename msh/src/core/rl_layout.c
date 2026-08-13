#include <stdio.h>
#include <stdlib.h>

#include "readline_internal.h"
#include "utf8.h"

/*
** ============================================================
** Terminal columns
** ============================================================
*/

static size_t	rl_term_cols(t_rl *rl)
{
	if (!rl || rl->terminal.cols == 0)
		return (RL_DEFAULT_TERM_COLS);

	return (rl->terminal.cols);
}

/*
** ============================================================
** Advance terminal-relative position
** ============================================================
**
** Координаты row/col здесь являются RELATIVE координатами
** readline.
**
** Но первый физический ряд начинается не обязательно с
** terminal column 0.
**
** Например:
**
**     terminal.cols = 29
**     origin_col    = 20
**
** тогда:
**
**     relative (0,0) -> absolute column 20
**     relative (0,1) -> absolute column 21
**     ...
**     relative (0,8) -> absolute column 28
**     relative (1,0) -> absolute column 0
**
** Поэтому первый ряд имеет только:
**
**     cols - origin_col
**
** доступных колонок.
**
** После первого переноса все последующие ряды имеют полную
** ширину terminal.cols.
**
** ВАЖНО:
**
** Эта функция НЕ превращает relative col в absolute col.
** Она только учитывает origin_col при определении места
** переноса.
** ============================================================
*/

static void	rl_advance(
		size_t *row,
		size_t *col,
		size_t width,
		size_t cols,
		size_t origin_col)
{
	size_t	first_cols;

	if (!row || !col || width == 0 || cols == 0)
		return;

	/*
	** Безопасность: origin_col должен находиться внутри
	** текущей строки терминала.
	**
	** Нормальная ситуация:
	**
	**     origin_col < cols
	**
	** Если по какой-либо причине это не так, начинаем
	** с полной строки.
	*/
	if (origin_col >= cols)
		origin_col = 0;

	/*
	** Сколько колонок доступно в первом физическом ряду.
	*/
	first_cols = cols - origin_col;

	/*
	** ========================================================
	** Первый readline-relative ряд
	** ========================================================
	*/
	if (*row == 0)
	{
		/*
		** Широкий символ не помещается в оставшееся место
		** первого ряда.
		*/
		if (width > first_cols - *col)
		{
			(*row)++;
			*col = 0;
		}
	}
	else
	{
		/*
		** Последующие ряды имеют полную ширину терминала.
		*/
		if (width > cols - *col)
		{
			(*row)++;
			*col = 0;
		}
	}

	*col += width;

	/*
	** Если символ занял последнюю доступную позицию,
	** readline-модель сразу переводится на следующий
	** физический ряд.
	**
	** Это соответствует rl_render.c.
	*/
	if (*row == 0)
	{
		if (*col >= first_cols)
		{
			(*row)++;
			*col = 0;
		}
	}
	else
	{
		if (*col >= cols)
		{
			(*row)++;
			*col = 0;
		}
	}
}

/*
** ============================================================
** Calculate text position
** ============================================================
**
** Считает позицию текста относительно начала readline.
**
** ВАЖНО:
**
** row/col НЕ являются абсолютными координатами терминала.
**
** origin_col используется только для определения того,
** сколько места осталось в первом физическом ряду.
** ============================================================
*/

static void	rl_text_position(
		const char *s,
		size_t bytes,
		size_t *row,
		size_t *col,
		size_t cols,
		size_t origin_col)
{
	size_t	pos;
	size_t	len;
	size_t	width;

	if (!s || !row || !col || cols == 0)
		return;

	pos = 0;

	while (pos < bytes)
	{
		/*
		** ANSI escape sequence.
		**
		** Она не занимает экранных колонок.
		*/
		if ((unsigned char)s[pos] == 0x1B)
		{
			len = utf8_ansi_len(s + pos);

			if (len == 0 || len > bytes - pos)
				len = 1;

			pos += len;
			continue;
		}

		/*
		** Newline.
		*/
		if (s[pos] == '\n')
		{
			(*row)++;
			*col = 0;
			pos++;
			continue;
		}

		/*
		** Carriage return.
		*/
		if (s[pos] == '\r')
		{
			*col = 0;
			pos++;
			continue;
		}

		/*
		** UTF-8 character.
		*/
		len = utf8_char_len(s + pos);

		if (len == 0 || len > bytes - pos)
		{
			len = 1;
			width = 1;
		}
		else
		{
			width = utf8_char_width(s + pos);

			if (width == 0)
				width = 1;
		}

		rl_advance(
			row,
			col,
			width,
			cols,
			origin_col);

		pos += len;
	}
}

/*
** ============================================================
** Calculate cursor position from byte position
** ============================================================
**
** Возвращает readline-relative координаты курсора.
**
** Например при:
**
**     terminal.cols = 29
**     origin_col    = 20
**
** и строке из 9 ASCII символов:
**
**     cursor = (1,0)
**
** а не:
**
**     cursor = (0,9)
**
** ============================================================
*/

void	rl_calc_position(
		t_rl *rl,
		size_t byte_pos,
		size_t *row,
		size_t *col)
{
	size_t	cols;
	size_t	origin_col;

	if (!rl || !row || !col)
		return;

	if (byte_pos > rl->line.buffer.len)
		byte_pos = rl->line.buffer.len;

	if (rl->line.buffer.data)
	{
		byte_pos = utf8_align_boundary(
			rl->line.buffer.data,
			byte_pos);
	}

	cols = rl_term_cols(rl);

	origin_col = rl->terminal.origin_col;

	if (origin_col >= cols)
		origin_col = 0;

	*row = 0;
	*col = 0;

	if (byte_pos == 0)
		return;

	rl_text_position(
		rl->line.buffer.data,
		byte_pos,
		row,
		col,
		cols,
		origin_col);
}

/*
** ============================================================
** Calculate complete layout
** ============================================================
**
** Layout рассчитывает:
**
**     1. конец всей отрисованной области;
**     2. позицию editable cursor;
**     3. количество физических строк.
**
** Все координаты layout остаются RELATIVE.
**
** Абсолютное положение получает только terminal.c через
** rl_abs_position().
** ============================================================
*/

void	rl_calc_layout(t_rl *rl)
{
	size_t	row;
	size_t	col;
	size_t	cols;
	size_t	origin_col;

	if (!rl)
		return;

	cols = rl_term_cols(rl);

	origin_col = rl->terminal.origin_col;

	if (origin_col >= cols)
		origin_col = 0;

	row = 0;
	col = 0;

	/*
	** ========================================================
	** Editable line
	** ========================================================
	*/

	if (rl->line.buffer.data && rl->line.buffer.len != 0)
	{
		rl_text_position(
			rl->line.buffer.data,
			rl->line.buffer.len,
			&row,
			&col,
			cols,
			origin_col);
	}

	/*
	** ========================================================
	** Suggestion follows editable line
	** ========================================================
	*/

	if (rl->suggestion.text && rl->suggestion.bytes != 0)
	{
		/*
		** Здесь origin_col уже должен использоваться только
		** для первого физического ряда всей render-area.
		**
		** Если editable line уже перешла на row > 0,
		** origin_col больше не влияет.
		*/
		rl_text_position(
			rl->suggestion.text,
			rl->suggestion.bytes,
			&row,
			&col,
			cols,
			origin_col);
	}

	rl->layout.end_row = row;
	rl->layout.end_col = col;

	/*
	** Даже если конечная позиция:
	**
	**     row=1 col=0
	**
	** это уже вторая физическая строка.
	*/
	rl->layout.rows = row + 1;

	if (rl->layout.rows == 0)
		rl->layout.rows = 1;

	/*
	** ========================================================
	** Display width of editable line
	** ========================================================
	*/

	rl->layout.line_cols = utf8_width_n(
		rl->line.buffer.data,
		rl->line.buffer.len);

	rl->layout.suggestion_cols =
		rl->suggestion.cols;

	/*
	** ========================================================
	** Debug
	** ========================================================
	*/

	if (getenv("MSH_CURSOR_DEBUG"))
	{
		fprintf(
			stderr,
			"LAYOUT_DEBUG: "
			"buffer_len=%zu "
			"line_cursor=%zu "
			"origin_col=%zu "
			"cols=%zu\n",
			rl->line.buffer.len,
			rl->line.cursor,
			origin_col,
			cols);
	}

	/*
	** ========================================================
	** Cursor is calculated from editable buffer only.
	** ========================================================
	*/

	rl_calc_position(
		rl,
		rl->line.cursor,
		&rl->layout.cursor_row,
		&rl->layout.cursor_col);
}

/*
** ============================================================
** Calculate number of rendered rows
** ============================================================
*/

size_t	rl_calc_rows(t_rl *rl)
{
	if (!rl)
		return (1);

	rl_calc_layout(rl);

	return (rl->layout.rows);
}