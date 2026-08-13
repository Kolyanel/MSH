#include <errno.h>

#include "readline_internal.h"

/*
** ============================================================
** Обработать обычный символ
** ============================================================
*/

static void rl_event_char(
	t_rl *rl,
	t_rl_event *ev)
{
	if (!rl || !ev || ev->len == 0)
		return;

	rl_insert(
		rl,
		ev->data,
		ev->len);
}

/*
** ============================================================
** Обработать Enter
** ============================================================
*/

static void rl_event_enter(
	t_rl *rl)
{
	if (!rl)
		return;

	rl->accepted = 1;
	rl->running = 0;

	rl_clear_suggestion(rl);
	rl->dirty = 1;
}

/*
** ============================================================
** Обработать EOF
** ============================================================
*/

static void rl_event_eof(
	t_rl *rl)
{
	if (!rl)
		return;

	/*
	** Ctrl-D на непустой строке не завершает readline.
	*/
	if (rl->line.buffer.len != 0)
		return;

	rl->accepted = 0;
	rl->running = 0;

	/*
	** errno == 0 означает настоящий EOF.
	*/
	errno = 0;
}

/*
** ============================================================
** Обработать Ctrl-C
** ============================================================
*/

static void rl_event_interrupt(
	t_rl *rl)
{
	if (!rl)
		return;

	rl->accepted = 0;
	rl->running = 0;

	/*
	** Для shell Ctrl-C должен означать прерывание
	** текущего readline, а не EOF.
	*/
	errno = EINTR;
}

/*
** ============================================================
** Обработать событие
** ============================================================
*/

void rl_process_event(
	t_rl *rl,
	t_rl_event *ev)
{
	if (!rl || !ev)
		return;

	switch (ev->key)
	{
	case RL_KEY_CHAR:
		rl_event_char(rl, ev);
		break;

	case RL_KEY_ENTER:
		rl_event_enter(rl);
		break;

	case RL_KEY_EOF:
		rl_event_eof(rl);
		break;

	case RL_KEY_INTERRUPT:
		rl_event_interrupt(rl);
		break;

	case RL_KEY_BACKSPACE:
	/*	dprintf(
			2,
			"EVENT BACKSPACE\n");*/
		rl_backspace(rl);
		break;

	case RL_KEY_DELETE:
		rl_delete(rl);
		break;

	case RL_KEY_LEFT:
		rl_cursor_left(rl);
		break;

	case RL_KEY_RIGHT:
		rl_cursor_right(rl);
		break;

	case RL_KEY_HOME:
		rl_cursor_home(rl);
		break;

	case RL_KEY_END:
		rl_cursor_end(rl);
		break;

	case RL_KEY_UP:
		rl_history_up(rl);
		break;

	case RL_KEY_DOWN:
		rl_history_down(rl);
		break;

	case RL_KEY_TAB:
		rl_complete(rl);
		break;

	case RL_KEY_NONE:
	default:
		break;
	}
}