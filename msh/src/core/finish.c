#include <unistd.h>

#include "readline_internal.h"


/*
** ============================================================
** Finish current input line
**
** Cursor is already stored by last redraw.
**
** ============================================================
*/

void rl_finish_line(
        t_rl *rl)
{
    static const char nl[] = "\r\n";


    if (!rl)
        return;


    /*
    ** Move cursor to current logical cursor position.
    */

    rl_move_cursor(
            rl,
            rl->render.start_row
                + rl->render.cursor_row,
            rl->render.start_col
                + rl->render.cursor_col);



    /*
    ** Finish line.
    */

    rl_write_all(
            rl->fd,
            nl,
            sizeof(nl) - 1);



    /*
    ** Invalidate render cache.
    */

    rl->render.rows = 0;
    rl->render.width = 0;

    rl->render.cursor_row = 0;
    rl->render.cursor_col = 0;

    rl->render.end_row = 0;
    rl->render.end_col = 0;

    rl->render.term_cursor_row = 0;
    rl->render.term_cursor_col = 0;


    rl->accepted = 1;
    rl->running = 0;
}