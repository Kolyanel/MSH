#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "readline_internal.h"


/*
** ============================================================
** Read terminal response
**
** Terminal returns:
**
** ESC [ row ; col R
**
** ============================================================
*/

static int rl_read_response(
        t_rl *rl,
        char *buf,
        size_t size)
{
    size_t i;
    char c;


    if (!rl || !buf || size < 2)
    {
        errno = EINVAL;
        return (-1);
    }


    i = 0;


    while (i < size - 1)
    {
        if (read(
                rl->fd,
                &c,
                1) != 1)
        {
            return (-1);
        }


        buf[i++] = c;


        if (c == 'R')
            break;
    }


    buf[i] = '\0';


    if (i == 0 || buf[i - 1] != 'R')
    {
        errno = EINVAL;
        return (-1);
    }


    return (0);
}



/*
** ============================================================
** Get cursor position
**
** Sends:
**
** ESC [ 6 n
**
** Receives:
**
** ESC [ row ; col R
**
** Coordinates returned zero based.
**
** ============================================================
*/

int rl_get_cursor_position(
        t_rl *rl,
        size_t *row,
        size_t *col)
{
    char response[64];
    int r;
    int c;


    if (!rl || rl->fd < 0 || !row || !col)
    {
        errno = EINVAL;
        return (-1);
    }



    if (rl_write_all(
            rl->fd,
            "\033[6n",
            4) < 0)
    {
        return (-1);
    }



    memset(
            response,
            0,
            sizeof(response));



    if (rl_read_response(
            rl,
            response,
            sizeof(response)) < 0)
    {
        return (-1);
    }



    if (sscanf(
            response,
            "\033[%d;%dR",
            &r,
            &c) != 2)
    {
        errno = EINVAL;
        return (-1);
    }



    if (r <= 0 || c <= 0)
    {
        errno = EINVAL;
        return (-1);
    }



    *row = (size_t)(r - 1);
    *col = (size_t)(c - 1);



    return (0);
}