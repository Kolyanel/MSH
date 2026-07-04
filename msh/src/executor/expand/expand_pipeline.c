#include <errno.h>

#include "expand.h"


t_exec_pipeline *expand_pipeline(t_exec_state *st, t_pipeline *src)
{
	if (!st || !src){
		errno = EINVAL;
		return NULL;
	}
	
	t_exec_pipeline *dst = exec_pipeline_create();
	
	if (!dst)
		return NULL;
	
	size_t cnt = vec_size(&src->commands);
	
	t_cmd **cmds = (t_cmd**) src->commands.val;
	
	for (size_t i = 0; i < cnt; ++i){
		
		t_exec_cmd *cmd = expand_command(st, cmds[i]);
		
		if (!cmd){
			exec_pipeline_free(dst);
			return NULL;
		}
		
		if (vec_push(&dst->commands, cmd) < 0){
			exec_cmd_free(cmd);
			exec_pipeline_free(dst);
			return NULL;
		}
	}
	return dst;
}