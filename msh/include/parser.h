#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "vector.h"
#include "ast.h"


/* --- parser state --- */

typedef struct s_parser
{
	Vector *tokens;
	size_t i;
	t_ast *ast;
} t_parser;


/* ---- result ---- */

typedef enum e_parse_res
{
	PARSE_ERR = -1,
	PARSE_OK = 0,
	PARSE_NO_MATCH = 1
} t_parse_res;


/* --- parser rule --- */

typedef t_parse_res (*t_parse_rule) (t_parser *p);


/* ----- API ----- */

t_parse_res parse(Vector *tokens, t_ast *ast);


/* --- dispatch --- */

t_parse_res parse_dispatch(t_parser *p);


#endif  // PARSER_H