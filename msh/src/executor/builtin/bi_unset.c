#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "env.h"
#include "env_utils.h"
#include "io.h"


static void uns_print_err(const char *arg)
{
	puts_fd(STDERR_FILENO, "unset: ", sizeof("unset: ") - 1);
	
	if (arg)
		puts_fd(STDERR_FILENO, arg, 0);
	
	puts_fd(STDERR_FILENO, ": not a valid identifier\n", 0);
}



int bi_unset(t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!ctx || !ctx->state || !ctx->state->env || !argv){
		errno = EINVAL;
		return 1;
	}
	
	if (argc <= 1)
		return 0;
	
	int status = 0;
	
	for (size_t i = 1; i < argc; ++i){
		
		if (!is_valid_identifier(argv[i])){
			uns_print_err(argv[i]);
			status = 1;
			continue;
		}
		
		int ret = env_unset(ctx->state->env, argv[i]);
		
		if (ret < 0){
			status = 1;
			continue;
		}
		
		ctx->state->env_dirty = true;
	}
	
	return status;
}