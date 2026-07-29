#include <string.h>

#include "readline_internal.h"
#include "utf8.h"


void	rl_replace_line(
		t_rl *rl,
		const char *s)
{
	size_t	len;


	if (!rl)
		return;


	if (!s)
		s = "";


	len = strlen(s);


	rl->line.buffer.len = 0;

	if (rl->line.buffer.data)
		rl->line.buffer.data[0] = '\0';


	if (buf_append_span(
			&rl->line.buffer,
			s,
			len) < 0)
	{
		rl->line.cursor = 0;
		rl_clear_suggestion(rl);
		rl->dirty = 1;
		return;
	}


	if (rl->line.buffer.data)
		rl->line.buffer.data[rl->line.buffer.len] = '\0';


	rl->line.cursor =
		rl->line.buffer.len;


	if (rl->line.buffer.data)
	{
		rl->line.cursor =
			utf8_align_boundary(
				rl->line.buffer.data,
				rl->line.cursor);
	}


	rl_clear_suggestion(rl);

	rl->dirty = 1;
}



void	rl_save_current_line(
		t_rl *rl)
{
	if (!rl)
		return;


	rl->history.saved_line.len = 0;


	if (rl->history.saved_line.data)
		rl->history.saved_line.data[0] = '\0';


	if (!rl->line.buffer.data)
		return;


	if (buf_append_span(
			&rl->history.saved_line,
			rl->line.buffer.data,
			rl->line.buffer.len) < 0)
	{
		rl->history.saved_line.len = 0;

		if (rl->history.saved_line.data)
			rl->history.saved_line.data[0] = '\0';

		return;
	}


	if (rl->history.saved_line.data)
		rl->history.saved_line.data[
			rl->history.saved_line.len] = '\0';
}



void	rl_free_saved_line(
		t_rl *rl)
{
	if (!rl)
		return;


	rl->history.saved_line.len = 0;


	if (rl->history.saved_line.data)
		rl->history.saved_line.data[0] = '\0';
}



void	rl_history_up(
		t_rl *rl)
{
	t_hist	*h;
	size_t	next;


	if (!rl)
		return;


	h = rl->history.hist;


	if (!h)
		return;


	if (h->size == 0)
		return;


	if (h->head >= HIST_MAX)
		h->head = 0;


	if (h->index >= HIST_MAX)
		h->index = h->head;



	if (h->index == h->head)
		rl_save_current_line(rl);



	next = h->index;


	if (next == 0)
		next = HIST_MAX - 1;
	else
		next--;



	while (!h->lines[next])
	{
		if (next == 0)
			next = HIST_MAX - 1;
		else
			next--;


		if (next == h->index)
			return;
	}



	h->index = next;


	rl_replace_line(
		rl,
		h->lines[next]);
}



void	rl_history_down(
		t_rl *rl)
{
	t_hist	*h;


	if (!rl)
		return;


	h = rl->history.hist;


	if (!h)
		return;


	if (h->size == 0)
		return;


	if (h->head >= HIST_MAX)
		h->head = 0;


	if (h->index >= HIST_MAX)
		h->index = h->head;



	if (h->index != h->head)
	{
		h->index++;

		if (h->index >= HIST_MAX)
			h->index = 0;
	}



	if (h->index == h->head)
	{
		rl_replace_line(
			rl,
			rl->history.saved_line.data
				? rl->history.saved_line.data
				: "");


		rl_free_saved_line(rl);

		return;
	}



	if (h->lines[h->index])
	{
		rl_replace_line(
			rl,
			h->lines[h->index]);
	}
}