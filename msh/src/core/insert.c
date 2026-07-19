#include "readline_internal.h"


/*
** ============================================================
** Insert data into current line
**
** cursor:
**      byte offset inside line buffer
**
** len:
**      number of bytes
**
** Cursor is moved by bytes.
** UTF-8 characters may occupy multiple bytes.
**
** ============================================================
*/

void rl_insert(
        t_rl *rl,
        const char *s,
        size_t len)
{
    if (!rl || !s || len == 0)
        return;


    if (rl->cursor > rl->line.len)
    {
        rl->cursor = rl->line.len;
    }



    if (buf_insert_span(
            &rl->line,
            rl->cursor,
            s,
            len) < 0)
    {
        return;
    }



    /*
    ** Cursor position is stored in bytes,
    ** not terminal columns.
    */

    rl->cursor += len;



    /*
    ** Old completion suggestion
    ** is no longer valid.
    */

    rl_clear_suggestion(rl);
}