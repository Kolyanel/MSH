#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "exec_pipeline_internal.h"
#include "exec_builtin.h"
#include "msh_debug.h"

int exec_path_resolve(t_exec_ctx *ctx, t_exec_process *pr);
int exec_path_find(t_exec_ctx *ctx, t_exec_process *pr);

int exec_pl_resolve(t_exec_ctx *ctx)
{
    assert(ctx);

    if (!ctx || !ctx->pipeline || !ctx->job || !ctx->job->processes.val)
        return (errno = EINVAL, -1);

    t_exec_process **procs = (t_exec_process**)ctx->job->processes.val;
    size_t proc_cnt = vec_size(&ctx->job->processes);

    DBG("resolve processes=%zu\n", proc_cnt);

    for (size_t i = 0; i < proc_cnt; ++i)
    {
        t_exec_process *pr = procs[i];

        if (!pr || !pr->argv || !pr->argv[0])
            return (errno = EINVAL, -1);

        DBG("resolve cmd=%s\n", pr->argv[0]);

        pr->builtin = NULL;
        free(pr->exec_path);
        pr->exec_path = NULL;
        pr->kind = EXEC_EXTERNAL;

        const t_builtin *bi = builtin_find(pr->argv[0]);

        if (bi)
        {
            DBG("builtin found: %s\n", bi->name);
            pr->kind = EXEC_BUILTIN;
            pr->builtin = bi;
            continue;
        }

        if (exec_path_resolve(ctx, pr) < 0)
        {
            DBG("resolve failed: %s\n", pr->argv[0]);
            return -1;
        }

        DBG("resolved: %s\n", pr->exec_path);
    }

    return 0;
}