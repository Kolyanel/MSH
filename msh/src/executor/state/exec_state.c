#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "exec_state.h"
#include "env.h"
#include "path.h"
#include "my_string.h"


t_exec_state *exec_state_create(void)
{
	t_exec_state *st = calloc(1, sizeof(*st));
	
	if (!st){
		errno = ENOMEM;
		return NULL;
	}
	
	st->env = NULL;
	
	st->path = NULL;
	
	st->envp = NULL;
	
	st->hist = NULL;
	
	st->env_dirty = true;
	st->interactive = true;
	
	st->last_status = 0;
	st->shell_pid = getpid();
	
	if (sig_init(&st->signals) < 0){
		free(st);
		return NULL;
	}
	
	if (job_ctrl_init(&st->job_ctrl) < 0){
		sig_destroy(&st->signals);
		free(st);
		return NULL;
	}
	
	if (sig_set(&st->signals) < 0){
		job_ctrl_cleanup(&st->job_ctrl);
		sig_destroy(&st->signals);
		free(st);
		return NULL;
	}
	
	return st;
}



void exec_state_free(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_state *st = ptr;
	
	sig_destroy(&st->signals);
	
	job_ctrl_cleanup(&st->job_ctrl);
	
	free_args(st->envp);
	
	free(st);
}



int state_update_envp(t_exec_state *st)
{
	if (!st || !st->env){
		errno = EINVAL;
		return -1;
	}
	
	free_args(st->envp);
	st->envp = NULL;
	
	st->envp = build_envp(st->env);
	
	if (!st->envp)
		return -1;
	
	return 0;
}



int state_update_path(t_exec_state *st)
{
	if (!st || !st->path || !st->env){
		errno = EINVAL;
		return -1;
	}
	
	return path_from_env(st->path, st->env);
}




int state_sync_env(t_exec_state *st)
{
	if (!st){
		errno = EINVAL;
		return -1;
	}
	
	if (state_update_path(st) < 0)
		return -1;
	
	if (state_update_envp(st) < 0)
		return -1;
	
	st->env_dirty = false;
	
	return 0;
}