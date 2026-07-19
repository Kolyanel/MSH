#include "readline_internal.h"

/*
** ============================================================
** Process one input event
**
** This function modifies only readline state.
**
** It never redraws screen.
**
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

            rl_insert(
                    rl,
                    ev->data,
                    ev->len);

            rl_suggest(rl);
            break;

        case RL_KEY_BACKSPACE:

            rl_backspace(rl);

            rl_suggest(rl);
            break;

        case RL_KEY_DELETE:

            rl_delete(rl);

            rl_suggest(rl);
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

            rl_suggest(rl);
            break;

        case RL_KEY_DOWN:

            rl_history_down(rl);

            rl_suggest(rl);
            break;

        case RL_KEY_TAB:

            if (rl->suggestion)
                rl_accept_suggestion(rl);
            else
                rl_complete(rl);

            rl_suggest(rl);
            break;

        case RL_KEY_ENTER:

            rl->accepted = 1;
            rl->running = 0;
            break;

                case RL_KEY_EOF:

            if (rl->line.len == 0)
            {
                rl->accepted = 0;
            }
            else
            {
                rl->accepted = 1;
            }

            rl->running = 0;

            break;

        case RL_KEY_NONE:
        default:

            break;
    }
}