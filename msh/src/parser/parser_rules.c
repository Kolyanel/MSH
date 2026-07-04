#include <stdlib.h>
#include <errno.h>

#include "parser_rules.h"
#include "parser_state.h"
#include "ast_builder.h"
#include "token.h"


static bool is_stop_token(t_tok_type type)
{
	return (
	type == TOK_PIPE || 
	type == TOK_AND || 
	type == TOK_OR || 
	type == TOK_BACKGROUND || 
	type == TOK_SEMI
	);
}



static int token_to_redir(t_tok_type type, t_redir_type *out)
{
	if (!out){
		errno = EINVAL;
		return -1;
	}
	
	if (type == TOK_REDIR_IN)
		*out = REDIR_IN;
	
	else if (type == TOK_REDIR_OUT)
		*out = REDIR_OUT;
	
	else if (type == TOK_REDIR_APPEND)
		*out = REDIR_APPEND;
	
	else if (type == TOK_HEREDOC)
		*out = REDIR_HEREDOC;
	
	else{
		errno = EINVAL;
		return -1;
	}
	return 0;
}



static bool is_redir_token(t_tok_type type)
{
	return (
	type == TOK_REDIR_IN || 
	type == TOK_REDIR_OUT || 
	type == TOK_REDIR_APPEND || 
	type == TOK_HEREDOC
	);
}



static bool cmd_is_empty(t_cmd *cmd)
{
	if (!cmd)
		return true;
	
	return (
	cmd->argv.argc == 0 && 
	cmd->redirs.argc == 0
	);
}



t_parse_res parse_simple_command(t_parser *p, t_cmd **out)
{
	if (!p || !out){
		errno = EINVAL;
		return PARSE_ERR;
	}
	
	*out = cmd_create();
	
	if (!*out)
		return PARSE_ERR;
	
	while (!parser_at_end(p)){
		
		t_token *t = parser_cur(p);
		
		if (is_stop_token(t->type))
			break;
		
		if (is_redir_token(t->type)){
			
			t_redir_type redir_type;
			if (token_to_redir(t->type, &redir_type) < 0)
				goto fail;
			
			if (parser_advance(p) < 0)
				goto fail;
			
			t_token *target = parser_cur(p);
			
			if (!target || target->type != TOK_WORD){
				errno = EINVAL;
				goto fail;
			}
			
			t_redir *r = redir_create(redir_type, target);
			
			if (!r)
				goto fail;
			
			if (cmd_add_redir(*out, r) < 0)
				goto fail;
			
			if (parser_advance(p) < 0)
				goto fail;
			
			continue;
		}
		
		if (t->type == TOK_WORD){
			
			if (cmd_add_arg(*out, t) < 0)
				goto fail;
			
			if (parser_advance(p) < 0)
				goto fail;
			
			continue;
		}
		
		errno = EINVAL;
		goto fail;
	}
	
	if (cmd_is_empty(*out)){
		cmd_free(*out);
		*out = NULL;
		return PARSE_NO_MATCH;
	}
	
	return PARSE_OK;

fail:
	cmd_free(*out);
	*out = NULL;
	return PARSE_ERR;
}



t_parse_res parse_pipeline(t_parser *p, t_pipeline **out)
{
	if (!p || !out){
		errno = EINVAL;
		return PARSE_ERR;
	}
	
	*out = pipeline_create();
	
	if (!*out)
		return PARSE_ERR;
	
	t_cmd *cmd = NULL;
	
	t_parse_res r = parse_simple_command(p, &cmd);
	
	if (r != PARSE_OK)
		goto fail;
	
	if (pipeline_add_cmd(*out, cmd) < 0)
		goto fail;
	
	cmd = NULL;
	
	while (!parser_at_end(p)){
		
		t_token *t = parser_cur(p);
		
		if (!t || t->type != TOK_PIPE)
			break;
		
		if (parser_advance(p) < 0)
			goto fail;
		
		r = parse_simple_command(p, &cmd);
		
		if (r != PARSE_OK)
			goto fail;
		
		if (pipeline_add_cmd(*out, cmd) < 0)
			goto fail;
		
		cmd = NULL;
	}
	return PARSE_OK;

fail:
	if (cmd)
		cmd_free(cmd);
	if (*out)
		pipeline_free(*out);
	*out = NULL;
	return PARSE_ERR;
}




t_parse_res parse_and_or(t_parser *p, t_and_or **out)
{
	if (!p || !out){
		errno = EINVAL;
		return PARSE_ERR;
	}
	
	*out = and_or_create();
	
	if (!*out)
		return PARSE_ERR;
	
	t_pipeline *pl = NULL;
	
	t_parse_res r = parse_pipeline(p, &pl);
	
	if (r != PARSE_OK)
		goto fail;
	
	if (and_or_add_pipeline(*out, pl) < 0)
		goto fail;
	
	pl = NULL;
	
	while (!parser_at_end(p)){
		
		t_token *t = parser_cur(p);
		
		if (!t)
			break;
		
		t_cmd_link op;
		
		if (t->type == TOK_AND)
			 op = CMD_LINK_AND;
		
		else if (t->type == TOK_OR)
			op = CMD_LINK_OR;
		
		else
			break;
		
		if (and_or_add_link(*out, op) < 0)
			goto fail;
		
		if (parser_advance(p) < 0)
			goto fail;
		
		r = parse_pipeline(p, &pl);
		
		if (r != PARSE_OK)
			goto fail;
		
		if (and_or_add_pipeline(*out, pl) < 0)
			goto fail;
		
		pl = NULL;
	}
	
	return PARSE_OK;
	
fail:
	if (pl)
		pipeline_free(pl);
	
	if (*out)
		and_or_free(*out);
	
	return PARSE_ERR;
}




t_parse_res parse_line(t_parser *p)
{
	if (!p || !p->ast){
		errno = EINVAL;
		return PARSE_ERR;
	}
	
	t_and_or *node = NULL;
	
	while (!parser_at_end(p)){
		
		t_parse_res r = parse_and_or(p, &node);
		
		if (r != PARSE_OK)
			goto fail;
		
		t_token *t = parser_cur(p);
		
		if (t && t->type == TOK_BACKGROUND){
			node->background = true;
			if (parser_advance(p) < 0)
				goto fail;
		}
		
		if (ast_add_node(p->ast, node) < 0)
			goto fail;
		
		node = NULL;
		
		if (parser_at_end(p))
			break;
		
		t = parser_cur(p);
		
		if (!t)
			break;
		
		if (t->type == TOK_SEMI){
			
			if (parser_advance(p) < 0)
				goto fail;
			
			continue;
		}
		
		errno = EINVAL;
		goto fail;
	}
	
	return PARSE_OK;
	
fail:
	if (node)
		and_or_free(node);
	return PARSE_ERR;
}