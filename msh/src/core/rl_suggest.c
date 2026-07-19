#include <stdlib.h>
#include <string.h>

#include "readline_internal.h"
#include "utf8.h"



/*
** ============================================================
** Clear autosuggestion
**
** ============================================================
*/

void rl_clear_suggestion(
        t_rl *rl)
{
    if (!rl)
        return;


    free(rl->suggestion);

    rl->suggestion = NULL;

    rl->suggestion_bytes = 0;
    rl->suggestion_cols = 0;
}



/*
** ============================================================
** Search history suggestion
**
** Finds latest history entry starting with current line.
**
** ============================================================
*/

void rl_suggest(
        t_rl *rl)
{
    size_t i;
    size_t index;
    size_t start;

    const char *line;


    if (!rl)
        return;


    rl_clear_suggestion(rl);



    if (!rl->hist)
        return;


    if (rl->hist->size == 0)
        return;


    if (!rl->line.data || rl->line.len == 0)
        return;



    /*
    ** History is circular buffer.
    */

    if (rl->hist->size < HIST_MAX)
        start = 0;
    else
        start = rl->hist->head;



    i = rl->hist->size;


    while (i > 0)
    {
        i--;


        index = (start + i) % HIST_MAX;


        line = rl->hist->lines[index];


        if (!line)
            continue;



        if (strncmp(
                line,
                rl->line.data,
                rl->line.len) != 0)
        {
            continue;
        }



        if (strlen(line) <= rl->line.len)
            continue;



        rl->suggestion = strdup(
                line + rl->line.len);


        if (!rl->suggestion)
            return;



        rl->suggestion_bytes =
                strlen(rl->suggestion);



        rl->suggestion_cols =
                utf8_display_width(
                        rl->suggestion);



        return;
    }
}



/*
** ============================================================
** Accept autosuggestion
**
** Append suggestion to editable buffer.
**
** ============================================================
*/

void rl_accept_suggestion(
        t_rl *rl)
{
    if (!rl)
        return;


    if (!rl->suggestion)
        return;


    /*
    ** Suggestion is only valid at end of line.
    */

    if (rl->cursor != rl->line.len)
        return;



    if (buf_append_span(
            &rl->line,
            rl->suggestion,
            rl->suggestion_bytes) < 0)
    {
        return;
    }



    rl->cursor =
            rl->line.len;



    rl_clear_suggestion(rl);
}