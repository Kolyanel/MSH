#include <string.h>

#include "readline_internal.h"


/*
** ============================================================
** Replace current line
**
** Replace editable buffer content.
**
** Cursor is stored as byte offset.
**
** ============================================================
*/

void rl_replace_line(
        t_rl *rl,
        const char *s)
{
    size_t len;


    if (!rl)
        return;


    if (!s)
        s = "";


    len = strlen(s);



    rl->line.len = 0;


    if (rl->line.data)
        rl->line.data[0] = '\0';



    if (buf_append_span(
            &rl->line,
            s,
            len) < 0)
    {
        return;
    }



    rl->cursor = len;


    rl_clear_suggestion(rl);
}



/*
** ============================================================
** Save current line
**
** Save user input before entering history.
**
** ============================================================
*/

void rl_save_current_line(
        t_rl *rl)
{
    if (!rl)
        return;



    rl->saved_line.len = 0;


    if (rl->saved_line.data)
        rl->saved_line.data[0] = '\0';



    if (!rl->line.data)
        return;



    if (buf_append_span(
            &rl->saved_line,
            rl->line.data,
            rl->line.len) < 0)
    {
        rl->saved_line.len = 0;

        if (rl->saved_line.data)
            rl->saved_line.data[0] = '\0';
    }
}



/*
** ============================================================
** Free saved line
**
** Release temporary history buffer.
**
** ============================================================
*/

void rl_free_saved_line(
        t_rl *rl)
{
    if (!rl)
        return;


    buf_free(
            &rl->saved_line);


    buf_init(
            &rl->saved_line);
}



/*
** ============================================================
** History UP
**
** Move to older command.
**
** ============================================================
*/

void rl_history_up(
        t_rl *rl)
{
    t_hist *h;


    if (!rl || !rl->hist)
        return;


    h = rl->hist;



    if (h->size == 0)
        return;



    /*
    ** First history movement.
    */

    if (h->index == h->head)
        rl_save_current_line(rl);



    /*
    ** Move backward.
    */

    if (h->index == 0)
        h->index = HIST_MAX - 1;
    else
        h->index--;



    if (!h->lines[h->index])
        return;



    rl_replace_line(
            rl,
            h->lines[h->index]);
}



/*
** ============================================================
** History DOWN
**
** Move to newer command.
**
** ============================================================
*/

void rl_history_down(
        t_rl *rl)
{
    t_hist *h;


    if (!rl || !rl->hist)
        return;


    h = rl->hist;



    if (h->size == 0)
        return;



    /*
    ** Already at newest position.
    */

    if (h->index == h->head)
        return;



    h->index++;



    if (h->index >= HIST_MAX)
        h->index = 0;



    /*
    ** Returned after newest command.
    */

    if (h->index == h->head)
    {
        rl_replace_line(
                rl,
                rl->saved_line.data
                    ? rl->saved_line.data
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