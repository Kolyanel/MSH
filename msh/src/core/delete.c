#include "readline_internal.h"
#include "utf8.h"



/*
** ============================================================
** Backspace
**
** Delete UTF-8 character before cursor.
**
** Cursor position is stored in bytes.
**
** ============================================================
*/

void rl_backspace(
        t_rl *rl)
{
    size_t prev;


    if (!rl)
        return;


    if (!rl->line.data)
        return;


    if (rl->cursor == 0)
        return;



    prev =
        utf8_prev(
                rl->line.data,
                rl->cursor);



    if (buf_delete_range(
            &rl->line,
            prev,
            rl->cursor - prev) < 0)
    {
        return;
    }



    rl->cursor = prev;



    /*
    ** Current suggestion became invalid.
    */
    rl_clear_suggestion(rl);
}



/*
** ============================================================
** Delete
**
** Delete UTF-8 character at cursor.
**
** Cursor does not move.
**
** ============================================================
*/

void rl_delete(
        t_rl *rl)
{
    size_t next;


    if (!rl)
        return;


    if (!rl->line.data)
        return;


    if (rl->cursor >= rl->line.len)
        return;



    next =
        utf8_next(
                rl->line.data,
                rl->cursor);



    if (next <= rl->cursor)
        return;



    if (buf_delete_range(
            &rl->line,
            rl->cursor,
            next - rl->cursor) < 0)
    {
        return;
    }



    /*
    ** Cursor stays at the same byte position.
    */
    rl_clear_suggestion(rl);
}