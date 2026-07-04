#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "io.h"
#include "env.h"
#include "env_utils.h"


static int export_one(Vector *env, const char *arg)
{
	if (!env || !arg){
		errno = EINVAL;
		return -1;
	}
	
	char *key = NULL;
	char *val = NULL;
	int mode = ASSIGN;
	
	int ret = parse_assignment(arg, &key, &val, &mode);
	
	if (ret == 0){
		
		if (!env_assign(env, key, val, mode))
			goto fail;
		
		if (env_export(env, key, 1) < 0)
			goto fail;
	}
	else{
		
		if (errno != EINVAL)
			goto fail;
		
		if (!is_valid_identifier(arg)){
			errno = EINVAL;
			goto fail;
		}
		
		if (!env_set(env, arg, NULL))
			goto fail;
		
		if (env_export(env, arg, 1) < 0)
			goto fail;
	}
	
	free(key);
	free(val);
	
	return 0;
	
fail:
	free(key);
	free(val);
	return -1;
}



static void exp_print_err(const char *arg)
{
	puts_fd(STDERR_FILENO, "export: ", sizeof("export: ") - 1);
	
	if (arg)
		puts_fd(STDERR_FILENO, arg, 0);
	
	puts_fd(STDERR_FILENO, ": not a valid identifier\n", 0);
}



int bi_export(t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!ctx || !ctx->state || !ctx->state->env || !argv){
		errno = EINVAL;
		return 1;
	}
	
	if (argc == 1){
		
		env_print_fd(ctx->state->env, STDOUT_FILENO, EXP_PRINT);
		return 0;
	}
	
	int status = 0;
	
	for (size_t i = 1; i < argc; ++i){
		
		if (export_one(ctx->state->env, argv[i]) == 0){
			ctx->state->env_dirty = true;
		}
		else{
			exp_print_err(argv[i]);
			status = 1;
		}
	}
	return status;
}