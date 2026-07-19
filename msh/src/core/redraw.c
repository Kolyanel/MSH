#include <string.h>

#include "readline_internal.h"
#include "utf8.h"


/*
** ============================================================
** Calculate position inside readline render block
**
** byte_pos:
**      byte offset inside editable line.
**
** Returns relative coordinates:
**
** row / col inside readline block.
**
** ============================================================
*/

void rl_calc_position(
        t_rl *rl,
        size_t byte_pos,
        size_t *row,
        size_t *col)
{
    size_t pos;
    size_t cols;
    size_t next;
    int width;


    if (!rl || !row || !col)
        return;


    *row = 0;
    *col = 0;


    cols = rl->render.term_cols;

    if (cols == 0)
        cols = RL_DEFAULT_TERM_COLS;



    /*
    ** Prompt occupies first columns.
    */

    *col = rl->prompt_cols;


    while (*col >= cols)
    {
        (*row)++;
        *col -= cols;
    }



    pos = 0;


    while (rl->line.data
        && pos < byte_pos)
    {
        width =
            utf8_char_width(
                    rl->line.data + pos);


        if (width <= 0)
            width = 1;



        if (*col + (size_t)width > cols)
        {
            (*row)++;
            *col = 0;
        }



        *col += (size_t)width;



        next =
            utf8_next(
                    rl->line.data,
                    pos);


        if (next <= pos)
            pos++;
        else
            pos = next;
    }



    if (*col >= cols)
    {
        (*row)++;
        *col = 0;
    }
}



/*
** ============================================================
** Calculate rendered block size
**
** ============================================================
*/

static void rl_calc_render_size(
        t_rl *rl,
        size_t *rows,
        size_t *width)
{
    size_t row;
    size_t col;
    size_t cols;


    if (!rl || !rows || !width)
        return;


    cols = rl->render.term_cols;

    if (cols == 0)
        cols = RL_DEFAULT_TERM_COLS;



    rl_calc_position(
            rl,
            rl->line.len,
            &row,
            &col);



    if (rl->suggestion)
    {
        col += rl->suggestion_cols;


        while (col >= cols)
        {
            row++;
            col -= cols;
        }
    }



    *rows = row + 1;
    *width = col;
}



/*
** ============================================================
** Redraw readline block
**
** ============================================================
*/

void rl_redraw(
        t_rl *rl)
{
    size_t rows;
    size_t width;

    size_t cursor_row;
    size_t cursor_col;


    if (!rl)
        return;


    if (!rl->render.initialized)
        return;



    /*
    ** Remove previous render.
    */

    if (rl->render.rows)
    {
        rl_clear_render(rl);
    }



    /*
    ** Update terminal dimensions.
    */

    rl_get_terminal_size(rl);



    /*
    ** Move to readline block start.
    */

    rl_move_cursor(
            rl,
            rl->render.start_row,
            rl->render.start_col);



    /*
    ** Draw prompt.
    */

    if (rl->prompt
        && rl->prompt_bytes)
    {
        rl_write_all(
                rl->fd,
                rl->prompt,
                rl->prompt_bytes);
    }



    /*
    ** Draw editable line.
    */

    if (rl->line.data
        && rl->line.len)
    {
        rl_write_all(
                rl->fd,
                rl->line.data,
                rl->line.len);
    }



    /*
    ** Draw suggestion.
    */

    if (rl->suggestion
        && rl->suggestion_bytes)
    {
        rl_write_all(
                rl->fd,
                "\033[2m",
                4);


        rl_write_all(
                rl->fd,
                rl->suggestion,
                rl->suggestion_bytes);


        rl_write_all(
                rl->fd,
                "\033[22m",
                5);
    }



    /*
    ** Store render size.
    */

    rl_calc_render_size(
            rl,
            &rows,
            &width);



    rl->render.rows = rows;
    rl->render.width = width;



    /*
    ** Store end position.
    */

    if (rows > 0)
    {
        rl->render.end_row =
                rl->render.start_row + rows - 1;

        rl->render.end_col =
                width;
    }



    /*
    ** Calculate logical cursor position.
    */

    rl_calc_position(
            rl,
            rl->cursor,
            &cursor_row,
            &cursor_col);



    rl->render.cursor_row = cursor_row;
    rl->render.cursor_col = cursor_col;



    /*
    ** Move terminal cursor.
    */

    rl_move_cursor(
            rl,
            rl->render.start_row + cursor_row,
            (cursor_row == 0)
                ? rl->render.start_col + cursor_col
                : cursor_col);



    /*
    ** Save real terminal position.
    */

    rl->render.term_cursor_row =
            rl->render.start_row + cursor_row;


    rl->render.term_cursor_col =
            (cursor_row == 0)
                ? rl->render.start_col + cursor_col
                : cursor_col;
}