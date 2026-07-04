#ifndef LEXER_CORE_H
#define LEXER_CORE_H

#include <stddef.h>

#include "vector.h"



typedef enum e_core_res
{
	CORE_ERR = -1,
	CORE_OK = 0,
	CORE_NO_MATCH = 1
} t_core_res;


typedef t_core_res (*t_lex_rule) (const char **s, Vector *tokens, size_t pos);


t_core_res lex_dispatch(const char **s, Vector *tokens, size_t pos);


#endif  // LEXER_CORE_H