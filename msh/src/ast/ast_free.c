#include <stdlib.h>
#include <string.h>

#include "ast.h"


void redir_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_redir *r = ptr;
	
	free(r);
}


void cmd_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_cmd *cmd = ptr;
	
	vec_free(&cmd->argv, NULL);
	
	vec_free(&cmd->redirs, redir_free);
	
	free(cmd);
}


void pipeline_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_pipeline *pl = ptr;
	
	vec_free(&pl->commands, cmd_free);
	
	free(pl);
}




void and_or_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_and_or *node = ptr;
	
	vec_free(&node->pipelines, pipeline_free);
	
	vec_free(&node->links, free);
	
	free(node);
}



void ast_free(t_ast *ast)
{
	if (!ast)
		return;
	
	vec_free(&ast->nodes, and_or_free);
	
	memset(ast, 0, sizeof(*ast));
}