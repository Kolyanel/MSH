#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "readline_internal.h"


/*
** ============================================================
** Read one byte from terminal
** ============================================================
*/

int rl_read_byte(
        t_rl *rl,
        char *c)
{
    ssize_t ret;


    if (!rl || rl->fd < 0 || !c)
    {
        errno = EINVAL;
        return (-1);
    }


    while (1)
    {
        ret = read(
                rl->fd,
                c,
                1);


        if (ret < 0)
        {
            if (errno == EINTR)
                continue;

            return (-1);
        }


        if (ret == 0)
            return (0);


        return (1);
    }
}



/*
** ============================================================
** Read escape sequence
**
** ESC [ A etc.
**
** Internal helper.
**
** ============================================================
*/

static int rl_read_escape(
        t_rl *rl,
        char *seq,
        size_t size)
{
    size_t i;
    char c;


    if (!rl || rl->fd < 0 || !seq || size < 2)
    {
        errno = EINVAL;
        return (-1);
    }


    i = 0;


    while (i < size - 1)
    {
        if (rl_read_byte(
                rl,
                &c) <= 0)
        {
            return (-1);
        }


        seq[i++] = c;


        /*
        ** CSI sequences finish
        ** with a letter.
        */

        if ((c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z'))
        {
            break;
        }
    }


    seq[i] = '\0';


    return ((int)i);
}



/*
** ============================================================
** Decode escape sequence
** ============================================================
*/

static void rl_decode_escape(
        const char *seq,
        t_rl_event *ev)
{
    if (!seq || !ev)
        return;


    ev->key = RL_KEY_NONE;


    if (strcmp(seq, "[A") == 0)
        ev->key = RL_KEY_UP;

    else if (strcmp(seq, "[B") == 0)
        ev->key = RL_KEY_DOWN;

    else if (strcmp(seq, "[C") == 0)
        ev->key = RL_KEY_RIGHT;

    else if (strcmp(seq, "[D") == 0)
        ev->key = RL_KEY_LEFT;


    else if (strcmp(seq, "[H") == 0
        || strcmp(seq, "[1~") == 0)
        ev->key = RL_KEY_HOME;


    else if (strcmp(seq, "[F") == 0
        || strcmp(seq, "[4~") == 0)
        ev->key = RL_KEY_END;


    else if (strcmp(seq, "[3~") == 0)
        ev->key = RL_KEY_DELETE;
}



/*
** ============================================================
** Read UTF-8 character
**
** First byte already received.
**
** ============================================================
*/

static int rl_read_char(
        t_rl *rl,
        char first,
        t_rl_event *ev)
{
    size_t need;
    size_t len;
    char c;


    if (!rl || !ev)
        return (-1);


    need = utf8_char_len(
            (unsigned char)first);


    if (need >= sizeof(ev->data))
        need = sizeof(ev->data) - 1;


    ev->data[0] = first;

    len = 1;


    while (len < need)
    {
        if (rl_read_byte(
                rl,
                &c) <= 0)
        {
            return (-1);
        }


        ev->data[len++] = c;
    }


    ev->data[len] = '\0';

    ev->len = len;


    return (0);
}



/*
** ============================================================
** Read key event
** ============================================================
*/

int rl_read_key(
        t_rl *rl,
        t_rl_event *ev)
{
    char c;


    if (!rl || rl->fd < 0 || !ev)
    {
        errno = EINVAL;
        return (-1);
    }


    memset(
            ev,
            0,
            sizeof(*ev));



    if (rl_read_byte(
            rl,
            &c) <= 0)
    {
        ev->key = RL_KEY_EOF;
        return (0);
    }



    /*
    ** Escape sequence.
    */

    if ((unsigned char)c == 27)
    {
        char seq[16];


        memset(
                seq,
                0,
                sizeof(seq));


        if (rl_read_escape(
                rl,
                seq,
                sizeof(seq)) < 0)
        {
            return (-1);
        }


        rl_decode_escape(
                seq,
                ev);


        return (0);
    }



    /*
    ** Enter.
    */

    if (c == '\n' || c == '\r')
    {
        ev->key = RL_KEY_ENTER;
        return (0);
    }



    /*
    ** Ctrl-D.
    */

    if ((unsigned char)c == 4)
    {
        ev->key = RL_KEY_EOF;
        return (0);
    }



    /*
    ** Backspace.
    */

    if ((unsigned char)c == 127
        || (unsigned char)c == 8)
    {
        ev->key = RL_KEY_BACKSPACE;
        return (0);
    }



    /*
    ** Tab.
    */

    if (c == '\t')
    {
        ev->key = RL_KEY_TAB;
        return (0);
    }



    /*
    ** UTF-8 character.
    */

    ev->key = RL_KEY_CHAR;


    if (rl_read_char(
            rl,
            c,
            ev) < 0)
    {
        ev->key = RL_KEY_NONE;
        return (-1);
    }


    return (0);
}