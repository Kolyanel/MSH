#include <string.h>
#include <errno.h>

#include "exec_builtin.h"


int bi_cd(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_export(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_exit(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_pwd(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_echo(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_env(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_unset(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_fg(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_bg(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_jobs(t_exec_ctx *ctx, char **argv, size_t argc);


int bi_history(t_exec_ctx *ctx, char **argv, size_t argc);



static const t_builtin g_builtin[] = {
	{"cd", bi_cd, true},
	{"export", bi_export, true},
	{"exit", bi_exit, true},
	{"pwd", bi_pwd, false},
	{"echo", bi_echo, false},
	{"env", bi_env, true},
	{"unset", bi_unset, true},
	{"fg", bi_fg, true},
	{"bg", bi_bg, true},
	{"jobs", bi_jobs, true},
	{"history", bi_history, true},
	{NULL, NULL, false}
};


const t_builtin *builtin_find(const char *cmd)
{
	if (!cmd || !cmd[0])
		return NULL;
	
	for (size_t i = 0; g_builtin[i].name; ++i){
		
		if (strcmp(g_builtin[i].name, cmd) == 0)
			return &g_builtin[i];
	}
	
	return NULL;
}



bool builtin_runs_in_parent(const t_builtin *bi)
{
	return (bi && bi->run_in_parent);
}



int builtin_exec(const t_builtin *bi, t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!bi || !bi->fn || !argv || !argv[0]){
		errno = EINVAL;
		return -1;
	}
	
	return bi->fn(ctx, argv, argc);
}



int builtin_normalize_status(int status)
{
	if (status < 0)
		return 1;

	return status;
}