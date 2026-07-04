#include <errno.h>

#include "parser.h"
#include "parser_rules.h"


static t_parse_rule g_parse_rules[] = {
	parse_line
};

static const size_t g_parse_rules_cnt = sizeof(g_parse_rules) / sizeof(*g_parse_rules);

t_parse_res parse_dispatch(t_parser *p)
{
	if (!p){
		errno = EINVAL;
		return PARSE_ERR;
	}
	
	for (size_t i = 0; i < g_parse_rules_cnt; ++i){
		
		t_parse_res r = g_parse_rules[i](p);
		
		if (r == PARSE_OK)
			return PARSE_OK;
		
		if (r == PARSE_ERR)
			return PARSE_ERR;
	}
	return PARSE_NO_MATCH;
}