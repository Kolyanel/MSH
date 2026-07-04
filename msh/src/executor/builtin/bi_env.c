#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "exec_ctx.h"
#include "exec_builtin.h"
#include "exec_argv.h"
#include "my_string.h"
#include "env.h"
#include "env_utils.h"
#include "io.h"


static void env_print_err(const char *arg)
{
	puts_fd(STDERR_FILENO, "env: ", sizeof("env: ") - 1);
	
	if (arg){
		puts_fd(STDERR_FILENO, arg, 0);
		
		puts_fd(STDERR_FILENO, ": ", 2);
	}
	
	puts_fd(STDERR_FILENO, strerror(errno), 0);
	
	puts_fd(STDERR_FILENO, "\n", 1);
}



static int parse_env_options(Vector *clone_env, char **argv, size_t argc, size_t *start_idx)
{
	if (!clone_env || !argv || !start_idx){
		errno = EINVAL;
		return -1;
	}
	
	size_t i = 1;
	
	while (i < argc){
		
		if (strcmp(argv[i], "-i") == 0){
			vec_free(clone_env, free_env);
			
			if (vec_init(clone_env, 8) < 0)
				return -1;
			i++;
		}
		
		else if (strcmp(argv[i], "-u") == 0){
			
			if (i + 1 >= argc){
				errno = EINVAL;
				return -1;
			}
			
			if (env_unset(clone_env, argv[i + 1]) < 0)
				return -1;
			
			i += 2;
		}
		
		else if (strcmp(argv[i], "--") == 0){
			i++;
			break;
		}
		
		else
			break;
	}
	
	*start_idx = i;
	return 0;
}



static int parse_env_args(Vector *clone_env, char **argv, size_t argc, size_t *start_idx)
{
	if (!clone_env || !argv || !start_idx){
		errno = EINVAL;
		return -1;
	}
	
	size_t i = *start_idx;
	
	while (i < argc){
		
		char *key = NULL;
		char *val = NULL;
		int mode = ASSIGN;
		
		if (parse_assignment(argv[i], &key, &val, &mode) < 0){
			
			if (errno == ENOENT)
				break;
			
			free(key);
			free(val);
			return -1;
		}
		
		if (!env_assign(clone_env, key, val, mode)){
			free(key);
			free(val);
			return -1;
		}
		
		if (env_export(clone_env, key, 1) < 0){
			free(key);
			free(val);
			return -1;
		}
		
		free(key);
		free(val);
		i++;
	}
	
	*start_idx = i;
	
	return 0;
}



int bi_env(t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!ctx || !ctx->state || !ctx->state->env || !argv){
		errno = EINVAL;
		return 1;
	}
	
	if (argc == 1){
		env_print_fd(ctx->state->env, STDOUT_FILENO, ENV_PRINT);
		return 0;
	}
	
	Vector clone_env;
	Vector tmp_path;
	t_exec_state tmp_st;
	int ret = 1;
	
	memset(&clone_env, 0, sizeof(clone_env));
	memset(&tmp_path, 0, sizeof(tmp_path));
	memset(&tmp_st, 0, sizeof(tmp_st));
	
	if (vec_init(&clone_env, 8) < 0){
		env_print_err(NULL);
		goto fail;
	}
	
	if (env_clone(&clone_env, ctx->state->env) < 0){
		env_print_err(NULL);
		goto fail;
	}
	
	if (vec_init(&tmp_path, 0) < 0){
		env_print_err(NULL);
		goto fail;
	}
	
	size_t start_idx = 1;
	
	if(parse_env_options(&clone_env, argv, argc, &start_idx) < 0){
		goto fail;
	}
	
	if (parse_env_args(&clone_env, argv, argc, &start_idx) < 0){
		goto fail;
	}
	
	if (start_idx < argc){
		
		tmp_st.env = &clone_env;
		tmp_st.path = &tmp_path;
		tmp_st.last_status = ctx->state->last_status;
		tmp_st.envp = NULL;
		tmp_st.env_dirty = true;
		
		if (exec_argv_run(&tmp_st, &argv[start_idx], ctx->result) < 0){
			env_print_err(argv[start_idx]);
			goto fail;
		}
		free_args(tmp_st.envp);
		tmp_st.envp = NULL;
	}
	
	else
		env_print_fd(&clone_env, STDOUT_FILENO, ENV_PRINT);
	
	ret = 0;

fail:
	free_args(tmp_st.envp);
	vec_free(&tmp_path, free);
	vec_free(&clone_env, free_env);
	return ret;
}