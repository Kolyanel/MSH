#include <ctype.h>
#include <errno.h>

#include "lexer.h"
#include "lexer_core.h"



static inline void skip_spaces(const char **s)
{
	if (!s || !*s)
		return;
	
	while (**s && isspace((unsigned char) **s))
		(*s)++;
}



t_lex_status lex_line(const char *input, Vector *tokens)
{
	if (!input || !tokens){
		errno = EINVAL;
		return LEX_ERR;
	}
	
	const char *s = input;
	
	while (*s){
		
		skip_spaces(&s);
		
		if (!*s)
			break;
		
		size_t pos = (size_t) (s - input);
		
		const char *prev = s;
		
		t_core_res r = lex_dispatch(&s, tokens, pos);
		
		if (r == CORE_ERR)
			return LEX_ERR;
		
		if (r == CORE_NO_MATCH){
			errno = EFAULT;
			return LEX_SYNTAX;
		}
		
		if (s == prev){
			errno = EFAULT;
			return LEX_ERR;
		}
	}
	
	return LEX_OK;
}