#include <stdlib.h>
#include <string.h>

#include "readline_internal.h"
#include "utf8.h"



/*
** ============================================================
** Clear autosuggestion
** ============================================================
*/

void	rl_clear_suggestion(
		t_rl *rl)
{
	int	changed;


	if (!rl)
		return;


	changed = (rl->suggestion.text != NULL);


	free(rl->suggestion.text);

	rl->suggestion.text = NULL;

	rl->suggestion.bytes = 0;

	rl->suggestion.cols = 0;


	if (changed)
		rl->dirty = 1;
}



/*
** ============================================================
** Find suggestion from history
** ============================================================
*/

void	rl_suggest(
		t_rl *rl)
{
	size_t			i;
	size_t			index;
	size_t			start;
	size_t			len;
	size_t			entry_len;

	const char		*entry;


	if (!rl)
		return;


	rl_clear_suggestion(rl);


	if (!rl->history.hist)
		return;


	if (rl->history.hist->size == 0)
		return;


	if (!rl->line.buffer.data
		|| rl->line.buffer.len == 0)
		return;


	len = utf8_align_boundary(
			rl->line.buffer.data,
			rl->line.buffer.len);



	if (rl->history.hist->size < HIST_MAX)
		start = 0;
	else
		start = rl->history.hist->head;



	i = rl->history.hist->size;


	while (i > 0)
	{
		i--;


		index = (start + i) % HIST_MAX;


		entry = rl->history.hist->lines[index];


		if (!entry)
			continue;


		entry_len = strlen(entry);


		if (entry_len <= len)
			continue;


		if (strncmp(
				entry,
				rl->line.buffer.data,
				len) != 0)
			continue;



		rl->suggestion.text = malloc(
				entry_len - len + 1);


		if (!rl->suggestion.text)
			return;



		memcpy(
			rl->suggestion.text,
			entry + len,
			entry_len - len);



		rl->suggestion.text[entry_len - len] = '\0';



		rl->suggestion.bytes =
			entry_len - len;


		rl->suggestion.cols =
			utf8_width_n(
				rl->suggestion.text,
				rl->suggestion.bytes);


		rl->dirty = 1;


		return;
	}
}



/*
** ============================================================
** Accept autosuggestion
** ============================================================
*/

void	rl_accept_suggestion(
		t_rl *rl)
{
	char	*text;
	size_t	bytes;


	if (!rl)
		return;


	if (!rl->suggestion.text)
		return;


	if (rl->line.cursor != rl->line.buffer.len)
		return;


	text = rl->suggestion.text;
	bytes = rl->suggestion.bytes;


	/*
	** Detach before insert.
	** insert() may clear suggestion.
	*/

	rl->suggestion.text = NULL;
	rl->suggestion.bytes = 0;
	rl->suggestion.cols = 0;


	rl_insert(
		rl,
		text,
		bytes);


	free(text);


	rl->dirty = 1;
}