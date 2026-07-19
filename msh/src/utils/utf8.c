#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "utf8.h"



/*
** ============================================================
** UTF-8 byte length
** ============================================================
*/

size_t utf8_char_len(
        unsigned char c)
{
    if ((c & 0x80) == 0)
        return (1);

    if ((c & 0xE0) == 0xC0)
        return (2);

    if ((c & 0xF0) == 0xE0)
        return (3);

    if ((c & 0xF8) == 0xF0)
        return (4);

    return (1);
}



/*
** ============================================================
** Skip ANSI escape sequence
** ============================================================
*/

static size_t utf8_skip_ansi(
        const char *s,
        size_t pos)
{
    if (!s)
        return pos;


    if ((unsigned char)s[pos] != 0x1b)
        return pos;


    if (s[pos + 1] != '[')
        return pos + 1;


    pos += 2;


    while (s[pos])
    {
        if ((s[pos] >= 'a' && s[pos] <= 'z')
            ||
            (s[pos] >= 'A' && s[pos] <= 'Z'))
        {
            return (pos + 1);
        }

        pos++;
    }


    return (pos);
}



/*
** ============================================================
** Decode UTF-8 character
** ============================================================
*/

int utf8_decode(
        const char *s,
        size_t size,
        t_utf8_char *out)
{
    mbstate_t state;
    wchar_t wc;
    size_t ret;


    if (!s || !out || size == 0)
    {
        errno = EINVAL;
        return (-1);
    }


    memset(
            &state,
            0,
            sizeof(state));


    ret = mbrtowc(
            &wc,
            s,
            size,
            &state);


    if (ret == (size_t)-1
        || ret == (size_t)-2
        || ret == 0)
    {
        return (-1);
    }


    out->codepoint = (uint32_t)wc;
    out->bytes = ret;


    out->width = wcwidth(wc);


    if (out->width < 0)
        out->width = 1;


    return (0);
}



/*
** ============================================================
** Next UTF-8 character
** ============================================================
*/

size_t utf8_next(
        const char *s,
        size_t pos)
{
    t_utf8_char ch;


    if (!s || !s[pos])
        return (pos);


    if ((unsigned char)s[pos] == 0x1b)
        return utf8_skip_ansi(
                s,
                pos);


    if (utf8_decode(
            s + pos,
            strlen(s + pos),
            &ch) < 0)
    {
        return (pos + 1);
    }


    return (pos + ch.bytes);
}



/*
** ============================================================
** Previous UTF-8 character
** ============================================================
*/

size_t utf8_prev(
        const char *s,
        size_t pos)
{
    if (!s || pos == 0)
        return (0);


    pos--;


    while (pos > 0
        && ((unsigned char)s[pos] & 0xC0) == 0x80)
    {
        pos--;
    }


    return (pos);
}



/*
** ============================================================
** Character width
** ============================================================
*/

int utf8_char_width(
        const char *s)
{
    t_utf8_char ch;


    if (!s)
        return (0);


    if (utf8_decode(
            s,
            strlen(s),
            &ch) < 0)
    {
        return (1);
    }


    return (ch.width);
}



/*
** ============================================================
** Display width
** ============================================================
*/

size_t utf8_display_width(
        const char *s)
{
    if (!s)
        return (0);


    return utf8_display_width_n(
            s,
            strlen(s));
}



/*
** ============================================================
** Display width with byte limit
** ============================================================
*/

size_t utf8_display_width_n(
        const char *s,
        size_t bytes)
{
    size_t pos;
    size_t width;
    t_utf8_char ch;


    if (!s)
        return (0);


    pos = 0;
    width = 0;


    while (s[pos] && pos < bytes)
    {
        if ((unsigned char)s[pos] == 0x1b)
        {
            pos = utf8_skip_ansi(
                    s,
                    pos);

            continue;
        }


        if (utf8_decode(
                s + pos,
                bytes - pos,
                &ch) < 0)
        {
            pos++;
            width++;
            continue;
        }


        width += ch.width;

        pos += ch.bytes;
    }


    return (width);
}