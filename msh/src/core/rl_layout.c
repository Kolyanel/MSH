#include "readline_internal.h"
#include "utf8.h"

/*
** ============================================================
** Terminal columns
** ============================================================
*/

static size_t	rl_term_cols(
				t_rl *rl)
{
	if (!rl || rl->terminal.cols == 0)
		return (RL_DEFAULT_TERM_COLS);
	return (rl->terminal.cols);
}

/*
** ============================================================
** First-line available columns
** ============================================================
**
** The prompt already occupies origin_col terminal columns.
**
** Therefore the first physical readline row has fewer
** available columns than subsequent rows.
**
** Example:
**
**     terminal width = 80
**     prompt width   = 20
**
**     first readline row = 60 columns
**     next rows          = 80 columns
**
** ============================================================
*/

static size_t	rl_first_line_cols(
				t_rl *rl)
{
	size_t	cols;

	cols = rl_term_cols(rl);

	if (rl->terminal.origin_col >= cols)
		return (1);

	return (cols - rl->terminal.origin_col);
}

/*
** ============================================================
** Advance visual position
** ============================================================
**
** row / col are RELATIVE to readline origin.
**
** On row zero, col is measured inside the available area
** after the prompt.
**
** On subsequent rows, col is measured from physical column 0.
**
** width is the visual width of one UTF-8 character.
**
** ============================================================
*/

static void	rl_advance(
				size_t *row,
				size_t *col,
				size_t width,
				size_t first_cols,
				size_t cols)
{
	size_t	available;

	if (!row || !col || width == 0 || cols == 0)
		return;

	/*
	** First physical readline row starts after the prompt.
	*/
	if (*row == 0)
		available = first_cols;
	else
		available = cols;

	/*
	** Defensive normalization.
	*/
	if (*col > available)
	{
		(*row)++;
		*col = 0;
		available = cols;
	}

	/*
	** Character does not fit on the current row.
	**
	** Do not split a UTF-8 character across terminal rows.
	*/
	if (width > available - *col)
	{
		(*row)++;
		*col = 0;
		available = cols;
	}

	/*
	** A character wider than the complete terminal width cannot
	** be represented without occupying the whole row.
	**
	** In normal UTF-8 terminal text this is not expected, but
	** keeping the position bounded makes the layout robust.
	*/
	if (width >= available && *col == 0)
	{
		*col = width;
		if (*col >= cols)
			*col = cols;
		return;
	}

	*col += width;

	/*
	** Reaching the right edge means the next character starts
	** on the following physical row.
	**
	** We keep col == available here. The next call to
	** rl_advance() performs the actual wrap.
	*/
}

/*
** ============================================================
** Text position
** ============================================================
**
** Coordinates are RELATIVE to readline origin.
**
** row:
**
**     0 = first physical row containing the editable input.
**
** col:
**
**     On row 0:
**         column relative to the position immediately after
**         the prompt.
**
**     On row > 0:
**         physical terminal column.
**
** The prompt is NOT included in the returned coordinates.
**
** ============================================================
*/

static void	rl_text_position(
				const char *s,
				size_t bytes,
				size_t *row,
				size_t *col,
				size_t first_cols,
				size_t cols)
{
	size_t	pos;
	size_t	ansi_len;
	size_t	char_len;
	size_t	width;

	if (!s || !row || !col || cols == 0)
		return;

	pos = 0;

	while (pos < bytes)
	{
		if ((unsigned char)s[pos] == 0x1B)
		{
			ansi_len = utf8_ansi_len(s + pos);

			if (ansi_len == 0)
			{
				pos++;
				continue;
			}

			if (ansi_len > bytes - pos)
			{
				pos++;
				continue;
			}

			pos += ansi_len;
			continue;
		}

		if (s[pos] == '\n')
		{
			(*row)++;
			*col = 0;
			pos++;
			continue;
		}

		if (s[pos] == '\r')
		{
			*col = 0;
			pos++;
			continue;
		}

		char_len = utf8_char_len(s + pos);

		if (char_len == 0 || char_len > bytes - pos)
		{
			char_len = 1;
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
			first_cols,
			cols);

		pos += char_len;
	}
}

/*
** ============================================================
** Calculate cursor position
** ============================================================
**
** byte_pos is a UTF-8 byte offset.
**
** Result is RELATIVE to readline origin.
**
** Therefore:
**
**     empty input:
**         row = 0
**         col = 0
**
**     first character:
**         starts immediately after prompt
**
** The prompt width is used only to calculate how many columns
** remain on the first physical row.
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
	size_t	first_cols;

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
	first_cols = rl_first_line_cols(rl);

	/*
	** Coordinates are relative to readline origin.
	*/
	*row = 0;
	*col = 0;

	if (!rl->line.buffer.data || byte_pos == 0)
		return;

	rl_text_position(
		rl->line.buffer.data,
		byte_pos,
		row,
		col,
		first_cols,
		cols);
}

/*
** ============================================================
** Calculate complete layout
** ============================================================
**
** All layout coordinates are RELATIVE to readline origin.
**
** The prompt is represented only by origin_col when calculating
** the available width of the first physical row.
**
** ============================================================
*/

void	rl_calc_layout(
				t_rl *rl)
{
	size_t	row;
	size_t	col;
	size_t	cols;
	size_t	first_cols;

	if (!rl)
		return;

	cols = rl_term_cols(rl);
	first_cols = rl_first_line_cols(rl);

	/*
	** Empty input begins at readline-relative (0, 0).
	*/
	row = 0;
	col = 0;

	if (rl->line.buffer.data
		&& rl->line.buffer.len != 0)
	{
		rl_text_position(
			rl->line.buffer.data,
			rl->line.buffer.len,
			&row,
			&col,
			first_cols,
			cols);
	}

	rl->layout.end_row = row;
	rl->layout.end_col = col;

	rl->layout.rows = row + 1;

	if (rl->layout.rows == 0)
		rl->layout.rows = 1;

	rl->layout.line_cols =
		utf8_width_n(
			rl->line.buffer.data,
			rl->line.buffer.len);

	rl->layout.suggestion_cols =
		rl->suggestion.cols;

	/*
	** Cursor position is calculated using the exact same
	** coordinate model as the rendered line.
	*/
	rl_calc_position(
		rl,
		rl->line.cursor,
		&rl->layout.cursor_row,
		&rl->layout.cursor_col);
}

/*
** ============================================================
** Calculate rendered rows
** ============================================================
**
** Calculates the physical rows occupied by:
**
**     line + suggestion
**
** Coordinates remain relative to readline origin.
**
** ============================================================
*/

size_t	rl_calc_rows(
				t_rl *rl)
{
	size_t	row;
	size_t	col;
	size_t	cols;
	size_t	first_cols;

	if (!rl)
		return (1);

	cols = rl_term_cols(rl);
	first_cols = rl_first_line_cols(rl);

	row = 0;
	col = 0;

	if (rl->line.buffer.data
		&& rl->line.buffer.len != 0)
	{
		rl_text_position(
			rl->line.buffer.data,
			rl->line.buffer.len,
			&row,
			&col,
			first_cols,
			cols);
	}

	if (rl->suggestion.text
		&& rl->suggestion.bytes != 0)
	{
		rl_text_position(
			rl->suggestion.text,
			rl->suggestion.bytes,
			&row,
			&col,
			first_cols,
			cols);
	}

	return (row + 1);
}