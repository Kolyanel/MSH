#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "env.h"
#include "io.h"


static int pwd_parse(char **argv, size_t argc, t_cd_mode *mode)
{
	if (!argv || !mode){
		errno = EINVAL;
		return -1;
	}
	
	*mode = CD_LOGICAL;
	
	for (size_t i = 1; i < argc; ++i){
		
		if (strcmp(argv[i], "--") == 0){
			if (i + 1 < argc){
				errno = EINVAL;
				return -1;
			}
			break;
		}
		
		else if (argv[i][0] == '-' && argv[i][1] != '\0' && argv[i][1] != '-'){
			
/* последний флаг L/P имеет приоритет*/
			for (size_t j = 1; argv[i][j]; ++j){
				
				if (argv[i][j] == 'L')
					*mode = CD_LOGICAL;
				
				else if (argv[i][j] == 'P')
					*mode = CD_PHYSICAL;
				
				else{
					errno = EINVAL;
					return -1;
				}
			}
		}
		
		else{
			errno = EINVAL;
			return -1;
		}
	}
	return 0;
}



static char *pwd_logical(t_exec_ctx *ctx)
{
	if (!ctx || !ctx->state || !ctx->state->env){
		errno = EINVAL;
		return NULL;
	}
	
	const char *pwd = env_get(ctx->state->env, "PWD");
	
	if (pwd && pwd[0])
		
		return strdup(pwd);
		
	return getcwd(NULL, 0);
}



static char *pwd_physical(void)
{
	return getcwd(NULL, 0);
}



static void pwd_print_err(const char *arg)
{
	puts_fd(STDERR_FILENO, "pwd: ", sizeof("pwd: ") - 1);
	
	if (arg){
		puts_fd(STDERR_FILENO, arg, 0);
		puts_fd(STDERR_FILENO, ": invalid option\n", 0);
	}
	else{
		puts_fd(STDERR_FILENO, strerror(errno), 0);
		puts_fd(STDERR_FILENO, "\n", 1);
	}
}



int bi_pwd(t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!ctx || !ctx->state || !argv){
		errno = EINVAL;
		return 1;
	}
	
	t_cd_mode mode;
	char *path = NULL;
	
	if (pwd_parse(argv, argc, &mode) < 0){
		pwd_print_err(argc > 1 ? argv[1] : NULL);
		return 1;
	}
	
	if (mode == CD_LOGICAL)
		path = pwd_logical(ctx);
	
	else
		path = pwd_physical();
	
	if (!path){
		pwd_print_err(NULL);
		return 1;
	}
	
	puts_fd(STDOUT_FILENO, path, 0);
	puts_fd(STDOUT_FILENO, "\n", 1);
	
	free(path);
	
	return 0;
}