#ifndef HANDLER_H
#define HANDLER_H

#include "token.h"


typedef int (*t_word_handler) (const char **s, t_token *tok);


/* --- text --- */

int handle_text(const char **s, t_token *tok);


/* --- escape --- */

int handle_escape(const char **s, t_token *tok);


/* -- $ вне кавычек -- */

int handle_variable(const char **s, t_token *tok);


/* --- quote --- */

int handle_single_quote(const char **s, t_token *tok);

int handle_double_quote(const char **s, t_token *tok);


/* --- dispatcher --- */

t_word_handler get_handler(char c);


#endif  // HANDLER_H