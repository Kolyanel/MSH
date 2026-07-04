#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "env.h"
#include "exec_ctx.h"
#include "exec_builtin.h"
#include "path_utils.h"
#include "my_string.h"
#include "memory.h"
#include "io.h"


static int cd_is_dir(const char *path)
{
	if (!path)
		return 0;
	
	struct stat st;
	
	if (stat(path, &st) < 0)
		return 0;
	
	return S_ISDIR(st.st_mode);
}



static char *cdpath_resolve(t_exec_ctx *ctx, const char *targ)
{
	if (!ctx || !ctx->state || !ctx->state->env || !targ){
		errno = EINVAL;
		return NULL;
	}
	
	const char *cdpath = env_get(ctx->state->env, "CDPATH");
	
	if (!cdpath)
		return NULL;
	
	char **dirs = parser_str(cdpath, ":");
	
	if (!dirs)
		return NULL;
	
	for (size_t i = 0; dirs[i]; ++i){
		
		char *try = cat_str_sep(dirs[i], targ, '/', malloc_wrap);
		
		if (!try)
			continue;
		
		if (cd_is_dir(try)){
			free_args(dirs);
			return try;
		}
		
		free(try);
	}
	
	free_args(dirs);
	return NULL;
}



static int cd_parse(t_exec_ctx *ctx, char **argv, size_t argc, t_cd_mode *mode, const char **targ)
{
	if (!ctx || !ctx->state || !ctx->state->env || !argv || !mode || !targ){
		errno = EINVAL;
		return -1;
	}
	
	*mode = CD_LOGICAL;
	
	if (argc == 1){
		
		*targ = env_get(ctx->state->env, "HOME");
		
		if (!*targ){
			errno = ENOENT;
			return -1;
		}
		return 0;
	}
	
	if (argc == 2 && strcmp(argv[1], "-") == 0){
		
		*targ = env_get(ctx->state->env, "OLDPWD");
		
		if (!*targ){
			errno = ENOENT;
			return -1;
		}
		
		return 0;
	}
	
	if (argc == 2 && argv[1][0] == '-'){
		
		if (strcmp(argv[1], "-P") == 0)
			*mode = CD_PHYSICAL;
		
		else if (strcmp(argv[1], "-L") == 0)
			*mode = CD_LOGICAL;
		
		else{
			errno = EINVAL;
			return -1;
		}
		*targ = env_get(ctx->state->env, "HOME");
		
		if (!*targ){
			errno = ENOENT;
			return -1;
		}
		
		return 0;
	}
	
	*targ = argv[1];
	
	if (argc == 3){
		
		if (strcmp(argv[1], "-P") == 0){
			
			*mode = CD_PHYSICAL;
			*targ = argv[2];
		}
		else if (strcmp(argv[1], "-L") == 0){
			*mode = CD_LOGICAL;
			*targ = argv[2];
		}
		else{
			
			errno = EINVAL;
			return -1;
		}
	}
	
	return 0;
}



static int cd_update_env(t_exec_ctx *ctx, const char *oldpwd, const char *newpwd)
{
	if (!ctx || !ctx->state || !ctx->state->env || !oldpwd || !newpwd){
		errno = EINVAL;
		return -1;
	}
	
	if (!env_set(ctx->state->env, "OLDPWD", oldpwd))
		return -1;
	
	if (env_export(ctx->state->env, "OLDPWD", 1) < 0)
		return -1;
	
	if (!env_set(ctx->state->env, "PWD", newpwd))
		return -1;
	
	if (env_export(ctx->state->env, "PWD", 1) < 0)
		return -1;
	
	return 0;
}



static int cd_change_dir(const char *path)
{
	if (!path){
		errno = EINVAL;
		return -1;
	}
	
	return chdir(path);
}


static void cd_print_err(const char *path)
{
	puts_fd(STDERR_FILENO, "cd: ", sizeof("cd: ") - 1);
	
	if (path){
		
		puts_fd(STDERR_FILENO, path, 0);
		
		puts_fd(STDERR_FILENO, ": ", sizeof(": ") - 1);
	}
	
	puts_fd(STDERR_FILENO, strerror(errno), 0);
	
	puts_fd(STDERR_FILENO, "\n", 1);
}




int bi_cd(t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!ctx || !ctx->state || !ctx->state->env || !argv){
		errno = EINVAL;
		return 1;
	}
	
	if (argc == 0 || argc > 3){
		
		puts_fd(STDERR_FILENO, "cd: incorrect number of arguments\n", 0);
		return 1;
	}
	
	t_cd_mode mode;
	const char *targ = NULL;
	
	if (cd_parse(ctx, argv, argc, &mode, &targ) < 0){
		
		cd_print_err(argc > 1 ? argv[1] : NULL);
		return 1;
	}
	
	char *cdpath_try = NULL;
	char *newpwd = NULL;
	char *oldpwd = NULL;
	
	const char *pwd = env_get(ctx->state->env, "PWD");
	
	if (!pwd)
		pwd = "/";
	
	oldpwd = strdup(pwd ? pwd : "/");
	
	if (!oldpwd){
		errno = ENOMEM;
		return 1;
	}
	
	if (mode == CD_LOGICAL && targ && targ[0] != '/' && targ[0] != '.'){
		
		cdpath_try = cdpath_resolve(ctx, targ);
		
		if (cdpath_try){
			puts_fd(STDOUT_FILENO, cdpath_try, 0);
			puts_fd(STDOUT_FILENO, "\n", 1);
			targ = cdpath_try;
		}
	}
	
	if (mode == CD_PHYSICAL){
		
		if (cd_change_dir(targ) < 0){
			
			cd_print_err(targ);
			goto fail;
		}
		
		newpwd = getcwd(NULL, 0);
		
		if (!newpwd){
			cd_print_err(targ);
			goto fail;
		}
	}
	else{
		
		newpwd = path_normalize(pwd, targ);
		if (!newpwd){
			
			cd_print_err(targ);
			goto fail;
		}
		
		if (cd_change_dir(newpwd) < 0){
			cd_print_err(targ);
			goto fail;
		}
	}
	
	if (cd_update_env(ctx, oldpwd, newpwd) < 0)
		goto fail;
	
	if (argc == 2 && strcmp(argv[1], "-") == 0){
		puts_fd(STDOUT_FILENO, newpwd, 0);
		
		puts_fd(STDOUT_FILENO, "\n", 1);
	}
	
	free(oldpwd);
	free(newpwd);
	free(cdpath_try);
	
	ctx->state->env_dirty = true;
	
	return 0;

fail:
	free(oldpwd);
	free(newpwd);
	free(cdpath_try);
	return 1;
}