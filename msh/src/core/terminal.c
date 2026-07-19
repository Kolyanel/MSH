#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "readline_internal.h"


/*
** ============================================================
** Enable raw terminal mode
**
** ============================================================
*/

int rl_enable_raw(
        t_rl *rl)
{
    struct termios raw;


    if (!rl || rl->fd < 0)
    {
        errno = EINVAL;
        return (-1);
    }


    if (tcgetattr(
            rl->fd,
            &rl->old_term) < 0)
    {
        return (-1);
    }


    raw = rl->old_term;


    raw.c_iflag &= ~(BRKINT
            | ICRNL
            | INLCR
            | IGNCR
            | IXON
            | IXOFF);


    raw.c_oflag &= ~(OPOST);


    raw.c_cflag |= CS8;


    raw.c_lflag &= ~(ECHO
            | ICANON
            | IEXTEN
            | ISIG);


    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;


    if (tcsetattr(
            rl->fd,
            TCSAFLUSH,
            &raw) < 0)
    {
        return (-1);
    }


    return (0);
}



/*
** ============================================================
** Restore terminal
**
** ============================================================
*/

void rl_restore(
        t_rl *rl)
{
    if (!rl || rl->fd < 0)
        return;


    tcsetattr(
            rl->fd,
            TCSAFLUSH,
            &rl->old_term);
}



/*
** ============================================================
** Write all bytes
**
** ============================================================
*/

int rl_write_all(
        int fd,
        const char *buf,
        size_t len)
{
    size_t off;
    ssize_t ret;


    if (fd < 0 || (!buf && len))
    {
        errno = EINVAL;
        return (-1);
    }


    off = 0;


    while (off < len)
    {
        ret = write(
                fd,
                buf + off,
                len - off);


        if (ret < 0)
        {
            if (errno == EINTR)
                continue;

            return (-1);
        }


        if (ret == 0)
        {
            errno = EIO;
            return (-1);
        }


        off += (size_t)ret;
    }


    return (0);
}



/*
** ============================================================
** Move cursor absolute
**
** Coordinates are zero based.
**
** ============================================================
*/

int rl_move_cursor(
        t_rl *rl,
        size_t row,
        size_t col)
{
    char seq[64];
    int len;


    if (!rl || rl->fd < 0)
    {
        errno = EINVAL;
        return (-1);
    }


    len = snprintf(
            seq,
            sizeof(seq),
            "\033[%zu;%zuH",
            row + 1,
            col + 1);


    if (len <= 0)
    {
        errno = EINVAL;
        return (-1);
    }


    return rl_write_all(
            rl->fd,
            seq,
            (size_t)len);
}



/*
** ============================================================
** Clear previous render area
**
** ============================================================
*/

void rl_clear_render(
        t_rl *rl)
{
    size_t i;


    if (!rl || rl->fd < 0)
        return;


    if (rl->render.rows == 0)
        return;



    /*
    ** Clear every rendered line.
    **
    ** Always move to column zero
    ** before erasing.
    */

    i = 0;


    while (i < rl->render.rows)
    {
        rl_move_cursor(
                rl,
                rl->render.start_row + i,
                0);


        rl_write_all(
                rl->fd,
                "\033[2K",
                4);


        i++;
    }



    /*
    ** Return to readline start.
    */

    rl_move_cursor(
            rl,
            rl->render.start_row,
            rl->render.start_col);



    /*
    ** Invalidate cache.
    */

    rl->render.rows = 0;
    rl->render.width = 0;

    rl->render.cursor_row = 0;
    rl->render.cursor_col = 0;

    rl->render.end_row = 0;
    rl->render.end_col = 0;



    /*
    ** Keep known terminal position.
    */

    rl->render.term_cursor_row =
            rl->render.start_row;

    rl->render.term_cursor_col =
            rl->render.start_col;
}