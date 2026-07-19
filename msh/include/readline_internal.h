#ifndef READLINE_INTERNAL_H
#define READLINE_INTERNAL_H

#include <stddef.h>
#include <termios.h>

#include "history.h"
#include "utf8.h"
#include "buf.h"



/*
** ============================================================
** Defaults
**
** ============================================================
*/

#define RL_DEFAULT_TERM_ROWS 24
#define RL_DEFAULT_TERM_COLS 80



/*
** ============================================================
** Keys
**
** ============================================================
*/

typedef enum e_rl_key
{
    RL_KEY_NONE = 0,

    RL_KEY_CHAR,

    RL_KEY_ENTER,
    RL_KEY_EOF,

    RL_KEY_BACKSPACE,
    RL_KEY_DELETE,

    RL_KEY_LEFT,
    RL_KEY_RIGHT,

    RL_KEY_HOME,
    RL_KEY_END,

    RL_KEY_UP,
    RL_KEY_DOWN,

    RL_KEY_TAB

} t_rl_key;



/*
** ============================================================
** Input event
**
** ============================================================
*/

typedef struct s_rl_event
{
    t_rl_key key;

    char data[5];

    size_t len;

} t_rl_event;



/*
** ============================================================
** Render state
**
** Stores renderer state.
** Coordinates are zero based.
**
** ============================================================
*/

typedef struct s_rl_render
{
    /*
    ** Start position of readline output.
    */
    size_t start_row;
    size_t start_col;



    /*
    ** Terminal size.
    */
    size_t term_rows;
    size_t term_cols;



    /*
    ** Previous rendered area.
    **
    ** rows  - number of terminal lines occupied.
    ** width - maximum visible width occupied.
    */
    size_t rows;
    size_t width;



    /*
    ** Cursor position relative
    ** to readline render block.
    */
    size_t cursor_row;
    size_t cursor_col;



    /*
    ** End position of rendered block.
    */
    size_t end_row;
    size_t end_col;



    /*
    ** Real terminal cursor position
    ** after drawing.
    */
    size_t term_cursor_row;
    size_t term_cursor_col;



    /*
    ** Renderer initialization state.
    **
    ** 0 - first render not initialized
    ** 1 - start position already known
    */
    int initialized;


} t_rl_render;



/*
** ============================================================
** Readline state
**
** ============================================================
*/

typedef struct s_rl
{
    /*
    ** Input/output descriptor.
    */
    int fd;



    /*
    ** Saved terminal attributes.
    */
    struct termios old_term;



    /*
    ** Prompt.
    */
    char *prompt;

    size_t prompt_bytes;
    size_t prompt_cols;



    /*
    ** Editable line.
    */
    t_buf line;

    size_t cursor;



    /*
    ** History.
    */
    t_hist *hist;



    /*
    ** Temporary saved line.
    */
    t_buf saved_line;



    /*
    ** Autosuggestion.
    */
    char *suggestion;

    size_t suggestion_bytes;
    size_t suggestion_cols;



    /*
    ** Renderer.
    */
    t_rl_render render;



    /*
    ** State flags.
    */
    int running;

    int accepted;


} t_rl;



/*
** ============================================================
** Lifecycle
**
** ============================================================
*/

int rl_init(
        t_rl *rl,
        int fd);


void rl_restore(
        t_rl *rl);


void rl_finish_line(
        t_rl *rl);



/*
** ============================================================
** Terminal
**
** ============================================================
*/

int rl_enable_raw(
        t_rl *rl);


int rl_write_all(
        int fd,
        const char *buf,
        size_t len);


int rl_move_cursor(
        t_rl *rl,
        size_t row,
        size_t col);


void rl_clear_render(
        t_rl *rl);


int rl_get_terminal_size(
        t_rl *rl);


int rl_get_cursor_position(
        t_rl *rl,
        size_t *row,
        size_t *col);



/*
** ============================================================
** Input
**
** ============================================================
*/

int rl_read_byte(
        t_rl *rl,
        char *c);


int rl_read_key(
        t_rl *rl,
        t_rl_event *ev);



/*
** ============================================================
** Rendering
**
** ============================================================
*/

void rl_redraw(
        t_rl *rl);


void rl_calc_position(
        t_rl *rl,
        size_t byte_pos,
        size_t *row,
        size_t *col);



/*
** ============================================================
** Editing
**
** ============================================================
*/

void rl_insert(
        t_rl *rl,
        const char *s,
        size_t len);


void rl_backspace(
        t_rl *rl);


void rl_delete(
        t_rl *rl);


void rl_cursor_left(
        t_rl *rl);


void rl_cursor_right(
        t_rl *rl);


void rl_cursor_home(
        t_rl *rl);


void rl_cursor_end(
        t_rl *rl);



/*
** ============================================================
** History
**
** ============================================================
*/

void rl_history_up(
        t_rl *rl);


void rl_history_down(
        t_rl *rl);


void rl_replace_line(
        t_rl *rl,
        const char *s);


void rl_save_current_line(
        t_rl *rl);


void rl_free_saved_line(
        t_rl *rl);



/*
** ============================================================
** Suggestion
**
** ============================================================
*/

void rl_suggest(
        t_rl *rl);


void rl_clear_suggestion(
        t_rl *rl);


void rl_accept_suggestion(
        t_rl *rl);



/*
** ============================================================
** Completion
**
** ============================================================
*/

void rl_complete(
        t_rl *rl);



/*
** ============================================================
** Event processing
**
** ============================================================
*/

void rl_process_event(
        t_rl *rl,
        t_rl_event *ev);



#endif