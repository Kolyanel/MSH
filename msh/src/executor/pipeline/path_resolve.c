#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "exec_pipeline_internal.h"


static char *build_path(const char *dir, const char *cmd)
{
	if (!cmd || !cmd[0]){
		errno = EINVAL;
		return NULL;
	}
	
	char *full = NULL;
	
	int ret;
	
	if (!dir || dir[0] == '\0'){
		
		ret = asprintf(&full, "./%s", cmd);
	}
	else{
		size_t dir_len = strlen(dir);
		
		while (dir_len > 0 && dir[dir_len - 1] == '/')
			dir_len--;
		
		if (dir_len == 0){
			ret = asprintf(&full, "./%s", cmd);
		} else{
			ret = asprintf(&full, "%.*s/%s", (int) dir_len, dir, cmd);
		}
	}
	if (ret < 0){
		errno = ENOMEM;
		return NULL;
	}
	
	return full;
}



int exec_path_resolve(t_exec_ctx *ctx, t_exec_process *pr)
{
	if (!ctx || !pr || !pr->argv || !pr->argv[0]){
		errno = EINVAL;
		return -1;
	}
	
	free(pr->exec_path);
	pr->exec_path = NULL;
	
	const char *cmd = pr->argv[0];
	
	if (!cmd[0]){
		errno = EINVAL;
		return -1;
	}
	
	if (strchr(cmd, '/')){
		
		pr->exec_path = strdup(cmd);
		
		if (!pr->exec_path){
			errno = ENOMEM;
			return -1;
		}
		
		if (access(pr->exec_path, X_OK) < 0){
			
			int saved = errno;
			free(pr->exec_path);
			pr->exec_path = NULL;
			
			errno = saved;
			return -1;
		}
		
		return 0;
	}
	
	return exec_path_find(ctx, pr);
}



int exec_path_find(t_exec_ctx *ctx, t_exec_process *pr)
{
	if (!ctx || !pr || !pr->argv || !pr->argv[0]){
		errno = EINVAL;
		return -1;
	}
	
	const char *cmd = pr->argv[0];
	
	Vector *path = ctx->state->path;
	
	if (!path || path->argc == 0){
		errno = ENOENT;
		return -1;
	}
	
	char **dirs = (char**) path->val;
	
	int best_errno = ENOENT;
	
	for (size_t i = 0; i < path->argc; ++i){
		
		if (!dirs[i])
			continue;
			
		char *full = build_path(dirs[i], cmd);
		
		if (!full)
			return -1;
		
		if (access(full, X_OK) == 0){
			free(pr->exec_path);
			pr->exec_path = full;
			return 0;
		}
			
		if (errno == EACCES)
			best_errno = EACCES;
		
		free(full);
		
	}
	
	errno = best_errno;
	return -1;
}