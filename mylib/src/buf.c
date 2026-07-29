#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"



int buf_reserve(
        t_buf *b,
        size_t need)
{
    size_t  required;
    size_t  new_cap;
    char    *tmp;


    if (!b)
    {
        errno = EINVAL;
        return (-1);
    }


    if (need > SIZE_MAX - b->len)
    {
        errno = ENOMEM;
        return (-1);
    }


    required = b->len + need;


    if (required <= b->cap)
        return (0);



    new_cap = b->cap;

    if (new_cap == 0)
        new_cap = BUF_INIT_CAP;



    while (new_cap < required)
    {
        if (new_cap > SIZE_MAX / 2)
        {
            new_cap = required;
            break;
        }

        new_cap *= 2;
    }



    if (new_cap > SIZE_MAX - 1)
    {
        errno = ENOMEM;
        return (-1);
    }



    tmp = realloc(
            b->data,
            new_cap + 1);


    if (!tmp)
    {
        errno = ENOMEM;
        return (-1);
    }


    b->data = tmp;
    b->cap = new_cap;


    return (0);
}



static int buf_inside(
        const t_buf *b,
        const char *s,
        size_t n)
{
    uintptr_t start;
    uintptr_t end;
    uintptr_t ptr;


    if (!b || !b->data || !s || n == 0)
        return (0);


    start = (uintptr_t)b->data;
    end = start + b->len;
    ptr = (uintptr_t)s;


    if (ptr < start || ptr > end)
        return (0);


    if (n > end - ptr)
        return (0);


    return (1);
}



int buf_init(
        t_buf *b)
{
    if (!b)
    {
        errno = EINVAL;
        return (-1);
    }


    b->data = malloc(
            BUF_INIT_CAP + 1);


    if (!b->data)
    {
        errno = ENOMEM;

        b->data = NULL;
        b->len = 0;
        b->cap = 0;

        return (-1);
    }


    b->data[0] = '\0';

    b->len = 0;
    b->cap = BUF_INIT_CAP;


    return (0);
}



int buf_clear(
        t_buf *b)
{
    if (!b || !b->data)
    {
        errno = EINVAL;
        return (-1);
    }


    b->len = 0;
    b->data[0] = '\0';


    return (0);
}



int buf_append(
        t_buf *b,
        char c)
{
    return (buf_append_span(
            b,
            &c,
            1));
}



int buf_append_span(
        t_buf *b,
        const char *s,
        size_t n)
{
    char *tmp;


    if (!b || !b->data || (!s && n != 0))
    {
        errno = EINVAL;
        return (-1);
    }


    if (n == 0)
        return (0);



    tmp = NULL;


    if (buf_inside(b, s, n))
    {
        tmp = malloc(n);

        if (!tmp)
        {
            errno = ENOMEM;
            return (-1);
        }


        memcpy(
                tmp,
                s,
                n);


        s = tmp;
    }



    if (buf_reserve(b, n) < 0)
    {
        free(tmp);
        return (-1);
    }



    memcpy(
            b->data + b->len,
            s,
            n);



    b->len += n;

    b->data[b->len] = '\0';


    free(tmp);

    return (0);
}



int buf_insert(
        t_buf *b,
        size_t pos,
        char c)
{
    return (buf_insert_span(
            b,
            pos,
            &c,
            1));
}



int buf_insert_span(
        t_buf *b,
        size_t pos,
        const char *s,
        size_t n)
{
    char *tmp;


    if (!b || !b->data
        || pos > b->len
        || (!s && n != 0))
    {
        errno = EINVAL;
        return (-1);
    }


    if (n == 0)
        return (0);



    tmp = NULL;


    if (buf_inside(b, s, n))
    {
        tmp = malloc(n);

        if (!tmp)
        {
            errno = ENOMEM;
            return (-1);
        }


        memcpy(
                tmp,
                s,
                n);


        s = tmp;
    }



    if (buf_reserve(b, n) < 0)
    {
        free(tmp);
        return (-1);
    }



    memmove(
            b->data + pos + n,
            b->data + pos,
            b->len - pos + 1);



    memcpy(
            b->data + pos,
            s,
            n);



    b->len += n;

    b->data[b->len] = '\0';


    free(tmp);

    return (0);
}



int buf_delete(
        t_buf *b,
        size_t pos)
{
    return (buf_delete_range(
            b,
            pos,
            1));
}



int buf_delete_range(
        t_buf *b,
        size_t pos,
        size_t n)
{
    if (!b || !b->data
        || pos > b->len
        || n > b->len - pos)
    {
        errno = EINVAL;
        return (-1);
    }


    if (n == 0)
        return (0);



    memmove(
            b->data + pos,
            b->data + pos + n,
            b->len - pos - n + 1);



    b->len -= n;

    b->data[b->len] = '\0';


    return (0);
}



char *buf_finalize(
        t_buf *b)
{
    char *res;


    if (!b || !b->data)
    {
        errno = EINVAL;
        return (NULL);
    }


    res = b->data;


    b->data = NULL;
    b->len = 0;
    b->cap = 0;


    return (res);
}



void buf_free(
        t_buf *b)
{
    if (!b)
        return;


    free(b->data);


    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}