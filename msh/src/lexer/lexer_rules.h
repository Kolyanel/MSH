#ifndef LEXER_RULES_H
#define LEXER_RULES_H

#include "lexer_core.h"


/* --- lex_operator --- */

t_core_res lex_logic(const char **s, Vector *tokens, size_t pos);

t_core_res lex_pipe(const char **s, Vector *tokens, size_t pos);

t_core_res lex_background(const char **s, Vector *tokens, size_t pos);

t_core_res lex_redir(const char **s, Vector *tokens, size_t pos);


/* ---- lex SEMI ---- */

t_core_res lex_semi(const char **s, Vector *tokens, size_t pos);


/* --- lex_word --- */

t_core_res lex_word(const char **s, Vector *tokens, size_t pos);


#endif  // LEXER_RULES_H