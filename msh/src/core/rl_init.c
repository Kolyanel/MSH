#include <errno.h>
#include <string.h>
#include <termios.h>

#include "readline_internal.h"


/*
** ============================================================
** Initialize readline state
**
** Prepare internal state.
**
** ============================================================
*/

int rl_init(
        t_rl *rl,
        int fd)
{
    if (!rl || fd < 0)
    {
        errno = EINVAL;
        return (-1);
    }



    memset(
            rl,
            0,
            sizeof(*rl));



    rl->fd = fd;



    /*
    ** Initialize editable buffer.
    */

    if (buf_init(
            &rl->line) < 0)
    {
        return (-1);
    }



    /*
    ** Initialize history temporary buffer.
    */

    if (buf_init(
            &rl->saved_line) < 0)
    {
        buf_free(
                &rl->line);

        return (-1);
    }



    /*
    ** Initialize render state.
    */

    memset(
            &rl->render,
            0,
            sizeof(rl->render));



    rl->render.term_rows =
            RL_DEFAULT_TERM_ROWS;

    rl->render.term_cols =
            RL_DEFAULT_TERM_COLS;



    /*
    ** Enable raw terminal mode.
    */

    if (rl_enable_raw(
            rl) < 0)
    {
        buf_free(
                &rl->saved_line);

        buf_free(
                &rl->line);

        return (-1);
    }



    /*
    ** Remove old pending input.
    **
    ** Needed before cursor position query.
    */

    tcflush(
            rl->fd,
            TCIFLUSH);



    /*
    ** Update terminal size.
    */

    rl_get_terminal_size(
            rl);



    /*
    ** Get current cursor position.
    **
    ** Renderer starts exactly where
    ** readline was requested.
    */

    if (rl_get_cursor_position(
            rl,
            &rl->render.start_row,
            &rl->render.start_col) < 0)
    {
        rl->render.start_row = 0;
        rl->render.start_col = 0;
    }



    rl->render.term_cursor_row =
            rl->render.start_row;

    rl->render.term_cursor_col =
            rl->render.start_col;



    /*
    ** Renderer initialized.
    */

    rl->render.initialized = 1;



    /*
    ** Prompt state.
    */

    rl->prompt = NULL;

    rl->prompt_bytes = 0;
    rl->prompt_cols = 0;



    /*
    ** Suggestion state.
    */

    rl->suggestion = NULL;

    rl->suggestion_bytes = 0;
    rl->suggestion_cols = 0;



    /*
    ** History.
    */

    rl->hist = NULL;



    /*
    ** Main loop state.
    */

    rl->running = 1;
    rl->accepted = 0;



    return (0);
}