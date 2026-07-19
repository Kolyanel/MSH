#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>


/*
** ============================================================
** UTF-8 character
** ============================================================
*/

typedef struct s_utf8_char
{
    uint32_t codepoint;

    /*
    ** Number of bytes occupied
    ** in UTF-8 sequence.
    */
    size_t bytes;


    /*
    ** Terminal display width:
    **
    ** 0 - combining/control
    ** 1 - normal character
    ** 2 - wide character
    */
    int width;

} t_utf8_char;



/*
** ============================================================
** UTF-8 byte length
**
** Returns expected sequence length
** from first byte.
**
** ============================================================
*/

size_t utf8_char_len(
        unsigned char c);



/*
** ============================================================
** Decode one UTF-8 character
**
** Return:
**
**  0 - success
** -1 - error
**
** ============================================================
*/

int utf8_decode(
        const char *s,
        size_t size,
        t_utf8_char *out);



/*
** ============================================================
** UTF-8 navigation
**
** Return byte offset of next/previous
** character.
**
** ============================================================
*/

size_t utf8_next(
        const char *s,
        size_t pos);


size_t utf8_prev(
        const char *s,
        size_t pos);



/*
** ============================================================
** Display width
**
** ANSI escape sequences ignored.
**
** ============================================================
*/

size_t utf8_display_width(
        const char *s);


size_t utf8_display_width_n(
        const char *s,
        size_t bytes);



/*
** ============================================================
** Single character display width
**
** ============================================================
*/

int utf8_char_width(
        const char *s);


#endif