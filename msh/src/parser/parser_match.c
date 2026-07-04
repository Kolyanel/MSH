#include "parser_match.h"
#include "parser_state.h"


bool parser_check(t_parser *p, t_tok_type type)
{
	if (!p)
		return false;
	
	t_token *tok = parser_cur(p);
	
	if (!tok)
		return false;
	
	return (tok->type == type);
}


bool parser_match(t_parser *p, t_tok_type type)
{
	if (!p)
		return false;
	
	if (!parser_check(p, type))
		return false;
	
	if (parser_advance(p) < 0)
		return false;
	
	return true;
}


t_parse_res parser_expect(t_parser *p, t_tok_type type)
{
	if (!p)
		return PARSE_ERR;
	
	if (!parser_match(p, type))
		return PARSE_NO_MATCH;
	
	return PARSE_OK;
}