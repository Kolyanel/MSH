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


/*
 * BUILTIN выполняемый только в родительском процессе.
 *
 * Важно:
 * builtin с pid != 0 уже был запущен через fork
 * (например из-за pipe или redirect).
 */
static bool is_parent_builtin_job(t_exec_job *job)
{
    if (!job || vec_size(&job->processes) != 1)
        return false;

    t_exec_process *pr =
        (t_exec_process *)job->processes.val[0];

    if (!pr)
        return false;

    return (pr->kind == EXEC_BUILTIN &&
            pr->pid <= 0);
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


    /*
     * Синхронизация окружения
     */
    if (st->env_dirty &&
        state_sync_env(st) < 0)
    {
        return EXEC_ERR;
    }


    t_exec_ctx ctx;


    if (exec_ctx_init(&ctx,
                      st,
                      pl,
                      &res->exec) < 0)
    {
        return EXEC_ERR;
    }


    ctx.job = exec_job_create();


    if (!ctx.job)
    {
        exec_ctx_free(&ctx);
        return EXEC_ERR;
    }


    ctx.job->foreground = !background;



    /*
     * Построение pipeline:
     *
     * parse
     * redir
     * pipe
     * spawn
     */
    for (size_t i = 0;
         i < g_stage_cnt;
         ++i)
    {
        if (g_stage[i](&ctx) < 0)
        {
            exec_job_destroy(ctx.job);
            ctx.job = NULL;

            exec_ctx_free(&ctx);
            return EXEC_ERR;
        }
    }



    if (!ctx.job ||
        vec_size(&ctx.job->processes) == 0)
    {
        if (ctx.job)
            exec_job_destroy(ctx.job);

        ctx.job = NULL;

        exec_ctx_free(&ctx);

        errno = EINVAL;
        return EXEC_ERR;
    }



    bool parent_builtin =
        is_parent_builtin_job(ctx.job);



    /*
     * Для обычного процесса pgid обязан быть создан.
     * Единственное исключение:
     * builtin в родителе.
     */
    if (ctx.job->pgid <= 0 &&
        !parent_builtin)
    {
        exec_job_destroy(ctx.job);
        ctx.job = NULL;

        exec_ctx_free(&ctx);

        errno = EINVAL;
        return EXEC_ERR;
    }



    if (job_ctrl_add(&st->job_ctrl,
                     ctx.job) < 0)
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



    /*
     * Builtin выполнен в родителе.
     * Ждать нечего.
     */
    if (parent_builtin)
    {
        res->exec.exit_code =
            job->exit_code;


        job_ctrl_remove(&st->job_ctrl,
                        job->job_id);


        exec_ctx_free(&ctx);

        return EXEC_OK;
    }



    /*
     * Background:
     * только регистрируем job.
     * Процессы уже созданы spawn stage.
     */
    if (background)
    {
        job->foreground = false;

        st->job_ctrl.last_bg_pid =
            job->pgid;


        printf_fd(STDOUT_FILENO,
                  "[%d] %d\n",
                  job->job_id,
                  job->pgid);


        res->exec.exit_code = 0;


        exec_ctx_free(&ctx);

        return EXEC_OK;
    }



    /*
     * Foreground ожидание
     */
    DBG_WAIT("foreground wait pgid=%d\n",
             job->pgid);


    if (wait_foreground(&st->job_ctrl,
                        job) < 0)
    {
        job_ctrl_remove(&st->job_ctrl,
                        job->job_id);

        exec_ctx_free(&ctx);

        return EXEC_ERR;
    }



    res->exec.exit_code =
        job->exit_code;



    if (job->state == JOB_DONE)
    {
        job_ctrl_remove(&st->job_ctrl,
                        job->job_id);
    }



    exec_ctx_free(&ctx);


    return EXEC_OK;
}