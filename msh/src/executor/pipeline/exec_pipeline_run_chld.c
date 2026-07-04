#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "exec_pipeline.h"
#include "exec_pipeline_internal.h"
#include "exec_wait.h"
#include "io.h"

extern t_exec_stage g_stage[];
extern const size_t g_stage_cnt;

t_exec_res exec_pipeline_run_child(t_exec_state *st,
                                   t_pipeline *pl,
                                   t_pipeline_res *res)
{
    if (!st || !pl || !res)
        return (errno = EINVAL, EXEC_ERR);

    memset(res, 0, sizeof(*res));

    if (st->env_dirty && state_sync_env(st) < 0)
        return EXEC_ERR;

    t_exec_ctx ctx;

    if (exec_ctx_init(&ctx, st, pl, &res->exec) < 0)
        return EXEC_ERR;

    ctx.job = exec_job_create();

    if (!ctx.job)
    {
        exec_ctx_free(&ctx);
        return EXEC_ERR;
    }

    for (size_t i = 0; i < g_stage_cnt; ++i)
    {
        if (g_stage[i](&ctx) < 0)
        {
            exec_job_destroy(ctx.job);
            exec_ctx_free(&ctx);
            return EXEC_ERR;
        }
    }

    if (!ctx.job || vec_size(&ctx.job->processes) == 0)
    {
        exec_job_destroy(ctx.job);
        exec_ctx_free(&ctx);
        errno = EINVAL;
        return EXEC_ERR;
    }

    ctx.job->foreground = true;

    t_exec_job *job = ctx.job;
    ctx.job = NULL;

    res->pgid = job->pgid;
    res->last_pid = ctx.last_pid;

    if (wait_foreground(&st->job_ctrl, job) < 0)
    {
        exec_ctx_free(&ctx);
        return EXEC_ERR;
    }

    res->exec.exit_code = job->exit_code;

    exec_ctx_free(&ctx);
    return EXEC_OK;
}