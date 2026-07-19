#include <errno.h>
#include <sys/ioctl.h>

#include "readline_internal.h"



/*
** ============================================================
** Get terminal size
**
** Updates only terminal dimensions.
**
** ============================================================
*/

int rl_get_terminal_size(
        t_rl *rl)
{
    struct winsize ws;


    if (!rl || rl->fd < 0)
    {
        errno = EINVAL;
        return (-1);
    }



    /*
    ** Always keep safe defaults.
    */

    rl->render.term_rows =
            RL_DEFAULT_TERM_ROWS;

    rl->render.term_cols =
            RL_DEFAULT_TERM_COLS;



    if (ioctl(
            rl->fd,
            TIOCGWINSZ,
            &ws) < 0)
    {
        return (-1);
    }



    if (ws.ws_row > 0)
    {
        rl->render.term_rows =
                ws.ws_row;
    }



    if (ws.ws_col > 0)
    {
        rl->render.term_cols =
                ws.ws_col;
    }



    return (0);
}