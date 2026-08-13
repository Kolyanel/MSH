#ifndef READLINE_INTERNAL_H
#define READLINE_INTERNAL_H

#include <stddef.h>
#include <termios.h>

#include "buf.h"
#include "history.h"


#define RL_DEFAULT_TERM_ROWS 24
#define RL_DEFAULT_TERM_COLS 80


typedef enum e_rl_key
{
	RL_KEY_NONE = 0,
	RL_KEY_CHAR,
	RL_KEY_ENTER,
	RL_KEY_EOF,
	RL_KEY_INTERRUPT,
	RL_KEY_BACKSPACE,
	RL_KEY_DELETE,
	RL_KEY_LEFT,
	RL_KEY_RIGHT,
	RL_KEY_HOME,
	RL_KEY_END,
	RL_KEY_UP,
	RL_KEY_DOWN,
	RL_KEY_TAB
}	t_rl_key;


typedef struct s_rl_event
{
	t_rl_key	key;
	char		data[5];
	size_t		len;
}	t_rl_event;


typedef struct s_rl_terminal
{
	size_t	rows;
	size_t	cols;

	size_t	origin_row;
	size_t	origin_col;

	size_t	cursor_row;
	size_t	cursor_col;

	size_t	draw_start_row;
	size_t	draw_start_col;

	size_t	draw_end_row;
	size_t	draw_end_col;

	size_t	draw_rows;

	int		raw_enabled;
	int		initialized;
}	t_rl_terminal;


typedef struct s_rl_layout
{
	size_t	rows;

	size_t	end_row;
	size_t	end_col;

	size_t	cursor_row;
	size_t	cursor_col;

	size_t	line_cols;
	size_t	suggestion_cols;
}	t_rl_layout;


typedef struct s_rl_line
{
	t_buf	buffer;
	size_t	cursor;
}	t_rl_line;


typedef struct s_rl_history_state
{
	t_hist	*hist;
	t_buf	saved_line;
	int		active;
}	t_rl_history_state;


typedef struct s_rl_suggestion
{
	char	*text;
	size_t	bytes;
	size_t	cols;
}	t_rl_suggestion;


typedef struct s_rl
{
	int					fd;
	struct termios		old_term;

	t_rl_terminal		terminal;
	t_rl_layout			layout;

	t_rl_line			line;
	t_rl_history_state	history;
	t_rl_suggestion		suggestion;

	int					running;
	int					accepted;
	int					dirty;
}	t_rl;


/*
** Lifecycle
*/

int		rl_init(t_rl *rl, int fd);
void	rl_restore(t_rl *rl);
void	rl_finish_line(t_rl *rl);
void	rl_destroy(t_rl *rl);


/*
** Terminal
*/

int		rl_enable_raw(t_rl *rl);
int		rl_write_all(int fd, const char *buf, size_t len);
int		rl_get_terminal_size(t_rl *rl);
int		rl_get_cursor_position(t_rl *rl, size_t *row, size_t *col);
int		rl_set_origin_from_terminal(t_rl *rl);
int		rl_move_cursor(t_rl *rl, size_t row, size_t col);
void	rl_clear_render(t_rl *rl);

void	rl_abs_position(
			t_rl *rl,
			size_t row,
			size_t col,
			size_t *abs_row,
			size_t *abs_col);


/*
** Input
*/

int		rl_read_byte(t_rl *rl, char *c);
int		rl_read_key(t_rl *rl, t_rl_event *ev);


/*
** Layout / Rendering
*/

void	rl_calc_layout(t_rl *rl);
void	rl_calc_position(
			t_rl *rl,
			size_t byte_pos,
			size_t *row,
			size_t *col);

size_t	rl_calc_rows(t_rl *rl);

void	rl_redraw(t_rl *rl);

int		rl_render_line(t_rl *rl);
int		rl_render_suggestion(t_rl *rl);


/*
** Editing
*/

int		rl_insert(t_rl *rl, const char *s, size_t len);
int		rl_delete_range(
			t_rl *rl,
			size_t start,
			size_t end);

void	rl_backspace(t_rl *rl);
void	rl_delete(t_rl *rl);

void	rl_cursor_left(t_rl *rl);
void	rl_cursor_right(t_rl *rl);
void	rl_cursor_home(t_rl *rl);
void	rl_cursor_end(t_rl *rl);


/*
** History
*/

void	rl_history_up(t_rl *rl);
void	rl_history_down(t_rl *rl);

void	rl_replace_line(
			t_rl *rl,
			const char *s);

void	rl_save_current_line(t_rl *rl);
void	rl_free_saved_line(t_rl *rl);


/*
** Suggestion
*/

void	rl_suggest(t_rl *rl);
void	rl_clear_suggestion(t_rl *rl);
void	rl_accept_suggestion(t_rl *rl);


/*
** Completion
*/

void	rl_complete(t_rl *rl);


/*
** Event processing
*/

void	rl_process_event(
			t_rl *rl,
			t_rl_event *ev);


/*
** Public readline entry point.
**
** Prompt is already printed by shell_loop().
*/
char	*readline_fd(
			int fd,
			const char *prompt,
			t_hist *hist);

#endif