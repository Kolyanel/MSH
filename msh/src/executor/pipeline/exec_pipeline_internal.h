#ifndef EXEC_PIPELINE_INTERNAL_H
#define EXEC_PIPELINE_INTERNAL_H

#include <stddef.h>

#include "exec_ctx.h"

int exec_pl_prepare(t_exec_ctx *ctx);
int exec_pl_build_processes(t_exec_ctx *ctx);
int exec_pl_resolve(t_exec_ctx *ctx);
int exec_pl_wire(t_exec_ctx *ctx);
int exec_pl_spawn(t_exec_ctx *ctx);
int exec_pl_wait(t_exec_ctx *ctx);

/* -- internal helpers -- */
int exec_pl_apply_process(t_exec_ctx *ctx, size_t i);
int exec_path_resolve(t_exec_ctx *ctx, t_exec_process *pr);
int exec_path_find(t_exec_ctx *ctx, t_exec_process *pr);

/* -- pipe cleanup -- */
void close_pipes(t_exec_ctx *ctx);


int exec_redir_apply(t_exec_process *pr);

/* builtin execution */
int builtin_exec(const t_builtin *bi,
                 t_exec_ctx *ctx,
                 char **argv,
                 size_t argc);

int builtin_normalize_status(int status);


#endif  // EXEC_PIPELINE_INTERNAL_H