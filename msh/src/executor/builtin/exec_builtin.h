#ifndef EXEC_BUILTIN_H
#define EXEC_BUILTIN_H

#include <stdbool.h>
#include <stddef.h>



typedef struct s_exec_ctx t_exec_ctx;

typedef struct s_exec_process t_exec_process;


typedef int (*t_builtin_fn) (t_exec_ctx *ctx, char **argv, size_t argc);


typedef struct s_builtin
{
	const char *name;
	t_builtin_fn fn;
	bool run_in_parent;
} t_builtin;


typedef enum e_cd_mode
{
	CD_LOGICAL,
	CD_PHYSICAL
} t_cd_mode;


const t_builtin *builtin_find(const char *cmd);

bool builtin_runs_in_parent(const t_builtin *bi);

int builtin_exec(const t_builtin *bi, t_exec_ctx *ctx, char **argv, size_t argc);


int builtin_normalize_status(int status);


int exec_builtin_parent(t_exec_ctx *ctx, t_exec_process *pr);


#endif  //EXEC_BUILTIN_H