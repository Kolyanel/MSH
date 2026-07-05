#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "exec_pipeline.h"
#include "exec_pipeline_internal.h"
#include "job_control.h"
#include "exec_wait.h"
#include "io.h"
#include "msh_debug.h"

extern t_exec_stage g_stage[];
extern const size_t g_stage_cnt;

static bool is_builtin_only_job(t_exec_job *job)
{
    if (!job || vec_size(&job->processes) != 1)
        return false;
    t_exec_process *pr = (t_exec_process *)job->processes.val[0];
    return (pr && pr->kind == EXEC_BUILTIN);
}

t_exec_res exec_pipeline_run(t_exec_state *st,
                             t_pipeline *pl,
                             bool background,
                             t_pipeline_res *res)
{
    assert(st);
    assert(pl);
    assert(res);

    if (!st || !pl || !res)
        return (errno = EINVAL, EXEC_ERR);

    memset(res, 0, sizeof(*res));

    /* ---------------- BACKGROUND ---------------- */
    if (background)
    {
        t_exec_job *job = exec_job_create();
        if (!job)
            return EXEC_ERR;

        pid_t pid = fork();
        if (pid < 0)
        {
            exec_job_destroy(job);
            return EXEC_ERR;
        }

        if (pid == 0)
        {
            t_pipeline_res child = {0};
            if (exec_pipeline_run(st, pl, false, &child) == EXEC_ERR)
                _exit(1);
            _exit(child.exec.exit_code);
        }

        job->pgid = pid;
        job->foreground = false;

        if (job_ctrl_add(&st->job_ctrl, job) < 0)
        {
            exec_job_destroy(job);
            return EXEC_ERR;
        }

        st->job_ctrl.last_bg_pid = pid;

        printf_fd(STDOUT_FILENO, "[%d] %d\n", job->job_id, pid);
        res->exec.exit_code = 0;
        return EXEC_OK;
    }

    /* ---------------- ENV ---------------- */
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

    ctx.job->foreground = true;

    /* ---------------- PIPELINE ---------------- */
    for (size_t i = 0; i < g_stage_cnt; ++i)
    {
        if (g_stage[i](&ctx) < 0)
        {
            exec_job_destroy(ctx.job);
            ctx.job = NULL;
            exec_ctx_free(&ctx);
            return EXEC_ERR;
        }
    }

    /* проверка: пустой список процессов */
    if (!ctx.job || vec_size(&ctx.job->processes) == 0)
    {
        if (ctx.job)
            exec_job_destroy(ctx.job);
        ctx.job = NULL;
        exec_ctx_free(&ctx);
        errno = EINVAL;
        return EXEC_ERR;
    }

    /* проверка: pgid <= 0 допустимо только для одиночного builtin */
    bool builtin_only = is_builtin_only_job(ctx.job);

    if (ctx.job->pgid <= 0 && !builtin_only)
    {
        exec_job_destroy(ctx.job);
        ctx.job = NULL;
        exec_ctx_free(&ctx);
        errno = EINVAL;
        return EXEC_ERR;
    }

    if (job_ctrl_add(&st->job_ctrl, ctx.job) < 0)
    {
        exec_job_destroy(ctx.job);
        ctx.job = NULL;
        exec_ctx_free(&ctx);
        return EXEC_ERR;
    }

    t_exec_job *job = ctx.job;
    ctx.job = NULL;

    res->pgid = job->pgid;
    res->last_pid = ctx.last_pid;

    /* builtin: выполнен в родителе, ждать нечего */
    if (builtin_only)
    {
        res->exec.exit_code = job->exit_code;
        job_ctrl_remove(&st->job_ctrl, job->job_id);
        exec_ctx_free(&ctx);
        return EXEC_OK;
    }

    DBG_WAIT("foreground wait pgid=%d\n", job->pgid);

    if (wait_foreground(&st->job_ctrl, job) < 0)
    {
        job_ctrl_remove(&st->job_ctrl, job->job_id);
        exec_ctx_free(&ctx);
        return EXEC_ERR;
    }

    res->exec.exit_code = job->exit_code;

    if (job->state == JOB_DONE)
        job_ctrl_remove(&st->job_ctrl, job->job_id);

    exec_ctx_free(&ctx);
    return EXEC_OK;
}