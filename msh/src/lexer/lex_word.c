#include <errno.h>

#include "lexer_rules.h"
#include "handler.h"
#include "vector.h"
#include "lex_utils.h"


t_core_res lex_word(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !*s || !tokens)
		return CORE_ERR;
	
	if (is_word_boundary(**s))
		return CORE_NO_MATCH;
	
	t_token *tok = token_new(TOK_WORD, pos);
	
	if (!tok)
		return CORE_ERR;
	
	while (**s && !is_word_boundary(**s)){
		
		const char *before = *s;
		
		t_word_handler handler = get_handler(**s);
		
		if (handler(s, tok) < 0)
			goto err;
		
		if (*s == before){
			errno = EINVAL;
			goto err;
		}
	}
	
	if (vec_push(tokens, tok) < 0)
		goto err;
		
	return CORE_OK;
	
err:
	token_free(tok);
	return CORE_ERR;
}