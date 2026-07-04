#include <errno.h>

#include "parser.h"
#include "parser_state.h"

t_parse_res parse(Vector *tokens, t_ast *ast)
{
	if (!tokens || !ast){
		errno = EINVAL;
		return PARSE_ERR;
	}
	
	t_parser p;
	
	if (parser_init(&p, tokens, ast) < 0)
		return PARSE_ERR;
	
	if (vec_init(&ast->nodes, 0) < 0)
		return PARSE_ERR;
	
	t_parse_res r = PARSE_OK;
	
	while (!parser_at_end(&p)){
		
		size_t old_i = p.i;
		
		r = parse_dispatch(&p);
		
		if (r == PARSE_OK && old_i == p.i){
			errno = EINVAL;
			r = PARSE_ERR;
			goto err;
		}
		
		if (r != PARSE_OK)
			goto err;
	}
	
	return PARSE_OK;

err:
	ast_free(ast);
	return r;
}