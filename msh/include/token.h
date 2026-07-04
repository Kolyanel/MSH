#ifndef TOKEN_H
#define TOKEN_H

#include "vector.h"

#include <stddef.h>


// тип токенов

typedef enum e_tok_type
{
	TOK_WORD, //слово
	TOK_PIPE, //  |
	TOK_AND, // И
	TOK_BACKGROUND, // &
	TOK_OR, // ИЛИ
	TOK_REDIR_IN, // <
	TOK_REDIR_OUT, // >
	TOK_REDIR_APPEND, // >>
	TOK_HEREDOC, // <<
	TOK_SEMI  // ;
} t_tok_type;


// токен

typedef struct s_token
{
	t_tok_type type;
	Vector parts;
	char *lexeme;
	size_t pos;
} t_token;



t_token *token_new(t_tok_type type, size_t pos);


void token_free(void *ptr);


/*
* собирает слово из частей
*/

char *token_to_str(t_token *t);


#endif  // TOKEN_H