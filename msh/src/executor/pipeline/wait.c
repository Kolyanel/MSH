#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>

#include "exec_pipeline_internal.h"
#include "exec_job.h"
#include "exec_wait.h"
#include "job_control.h"

int exec_pl_wait(t_exec_ctx *ctx)
{
    if (!ctx || !ctx->job || !ctx->job->processes.val)
    {
        errno = EINVAL;
        return -1;
    }

    t_exec_job *job = ctx->job;

    /* для одиночного builtin ждать нечего */
    if (job->pgid <= 0 && vec_size(&job->processes) == 1)
    {
        t_exec_process *pr = (t_exec_process *)job->processes.val[0];
        if (pr && pr->kind == EXEC_BUILTIN)
        {
            update_job_state(job);
            return job->exit_code;
        }
    }

    int status = wait_job(job, WUNTRACED | WCONTINUED);

    if (status < 0)
        return -1;

    if (update_job_state(job) < 0)
        return -1;

    if (job->state == JOB_DONE)
        job_ctrl_remove(&ctx->state->job_ctrl, job->job_id);

    return 0;
}