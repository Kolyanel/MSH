#include "lexer_core.h"
#include "lexer_rules.h"


extern t_lex_rule g_rules[];

extern const size_t g_rules_cnt;


t_core_res lex_dispatch(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !tokens)
		return CORE_ERR;
	
	for (size_t i = 0; i < g_rules_cnt; ++i){
		
		t_core_res r = g_rules[i](s, tokens, pos);
		
		if (r == CORE_OK)
			return CORE_OK;
		
		if (r == CORE_ERR)
			return CORE_ERR;
	}
	
	return CORE_NO_MATCH;
}