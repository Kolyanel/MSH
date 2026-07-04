#include <stdlib.h>
#include <errno.h>

#include "ast_builder.h"


t_pipeline *pipeline_create(void)
{
	t_pipeline *pl = calloc(1, sizeof(*pl));
	
	if (!pl){
		errno = ENOMEM;
		return NULL;
	}
	
	if (vec_init(&pl->commands, 0) < 0){
		free(pl);
		return NULL;
	}
	
	return pl;
}



t_cmd *cmd_create(void)
{
	t_cmd *cmd = calloc(1, sizeof(*cmd));
	
	if (!cmd){
		errno = ENOMEM;
		return NULL;
	}
	
	if (vec_init(&cmd->argv, 0) < 0){
		free(cmd);
		return NULL;
	}
	
	if (vec_init(&cmd->redirs, 0) < 0){
		vec_free(&cmd->argv, NULL);
		free(cmd);
		return NULL;
	}
	
	return cmd;
}



t_redir *redir_create(t_redir_type type, t_token *target)
{
	if (!target){
		errno = EINVAL;
		return NULL;
	}
	
	t_redir *r = calloc(1, sizeof(*r));
	
	if (!r){
		errno = ENOMEM;
		return NULL;
	}
	
	r->type = type;
	r->target = target;
	
	return r;
}




t_and_or *and_or_create(void)
{
	
	t_and_or *node = calloc(1, sizeof(*node));
	
	if (!node){
		errno = ENOMEM;
		return NULL;
	}
	
	if (vec_init(&node->pipelines, 0) < 0)
		goto fail;
	
	if (vec_init(&node->links, 0) < 0)
		goto fail;
	
	node->background = false;
	
	return node;
	
fail:
	vec_free(&node->pipelines, pipeline_free);
	vec_free(&node->links, free);
	free(node);
	return NULL;
}




int and_or_add_pipeline(t_and_or *node, t_pipeline *pl)
{
	if (!node || !pl){
		errno = EINVAL;
		return -1;
	}
	
	if (vec_push(&node->pipelines, pl) < 0){
		pipeline_free(pl);
		return -1;
	}
	
	return 0;
}




int and_or_add_link(t_and_or *node, t_cmd_link op)
{
	if (!node){
		errno = EINVAL;
		return -1;
	}
	
	t_cmd_link *link = malloc(sizeof(*link));
	
	if (!link){
		errno = ENOMEM;
		return -1;
	}
	
	*link = op;
	
	if (vec_push(&node->links, link) < 0){
		free(link);
		return -1;
	}
	
	return 0;
}



int cmd_add_arg(t_cmd *cmd, t_token *arg)
{
	if (!cmd || ! arg){
		errno = EINVAL;
		return -1;
	}
	
	if (vec_push(&cmd->argv, arg) < 0)
		return -1;
	
	return 0;
}



int cmd_add_redir(t_cmd *cmd, t_redir *redir)
{
	if (!cmd || !redir){
		errno = EINVAL;
		return -1;
	}
	
	if (vec_push(&cmd->redirs, redir) < 0){
		redir_free(redir);
		return -1;
	}
	return 0;
}



int pipeline_add_cmd(t_pipeline *pl, t_cmd *cmd)
{
	if (!pl || !cmd){
		errno = EINVAL;
		return -1;
	}
	
	if (vec_push(&pl->commands, cmd) < 0){
		cmd_free(cmd);
		return -1;
	}
	return 0;
}



int ast_add_node(t_ast *ast, t_and_or *node)
{
	if (!ast || !node){
		errno = EINVAL;
		return -1;
	}
	
	if (vec_push(&ast->nodes, node) < 0){
		and_or_free(node);
		return -1;
	}
	return 0;
}