#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "exec_cmd.h"
#include "my_string.h"


t_exec_redir *exec_redir_create(t_redir_type type, char *target)
{
	if (!target){
		errno = EINVAL;
		return NULL;
	}
	
	t_exec_redir *r = calloc(1, sizeof(*r));
	
	if (!r)
		return NULL;
	
	r->type = type;
	r->target = target;
	r->heredoc_fd = -1;
	
	return r;
}


void exec_redir_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_redir *r = ptr;
	
	free(r->target);
	if (r->heredoc_fd >= 0){
		close(r->heredoc_fd);
		r->heredoc_fd = -1;
	}
	free(r);
}



t_exec_cmd *exec_cmd_create(void)
{
	t_exec_cmd *cm = calloc(1, sizeof(*cm));
	
	if (!cm)
		return NULL;
	
	cm->argv = NULL;
	
	if (vec_init(&cm->redirs, 0) < 0){
		free(cm);
		return NULL;
	}
	
	return cm;
}



void exec_cmd_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_cmd *cm = ptr;
	
	vec_free(&cm->redirs, exec_redir_free);
	
	if (cm->argv)
		free_args(cm->argv);
	
	free(cm);
}



t_exec_pipeline *exec_pipeline_create(void)
{
	t_exec_pipeline *pl = calloc(1, sizeof(*pl));
	
	if (!pl)
		return NULL;
	
	pl->link = CMD_LINK_NONE;
	
	if (vec_init(&pl->commands, 8) < 0){
		free(pl);
		return NULL;
	}
	
	return pl;
}



void exec_pipeline_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_pipeline *pl = ptr;
	
	vec_free(&pl->commands, exec_cmd_free);
	
	free(pl);
}