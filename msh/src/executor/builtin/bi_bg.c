#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "job_control.h"
#include "io.h"


static int bg_error(const char *spec, const char *msg)
{
    if (spec) {
        printf_fd(STDERR_FILENO, "bg: %s: %s\n", spec, msg);
    } else {
        printf_fd(STDERR_FILENO, "bg: %s\n", msg);
    }
    return 1;
}


int bi_bg(t_exec_ctx *ctx, char **argv, size_t argc)
{
    if (!ctx || !ctx->state) {
        errno = EINVAL;
        return 1;
    }

    t_job_control *jc = &ctx->state->job_ctrl;
    const char *spec = (argc > 1) ? argv[1] : NULL;

    t_exec_job *job = NULL;

    /* Если нет аргументов — берём последний остановленный job */
    if (argc == 1) {
        job = job_ctrl_find_last_stopped(jc);
        if (!job) {
            job = job_ctrl_find_last(jc);
        }
        if (!job) {
            puts_fd(STDERR_FILENO, "bg: no current job\n", 0);
            return 1;
        }
    } else {
        job = job_lookup(jc, spec);
        if (!job) {
            return bg_error(spec, "no such job");
        }
    }

    if (job->state == JOB_DONE) {
        return bg_error(spec, "job already done");
    }

    /* Отправляем SIGCONT, чтобы продолжить остановленный процесс */
    if (job->pgid > 0) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            return bg_error(spec, "failed to continue job");
        }
    }

    /* Переводим в background */
    if (job_ctrl_set_bg(jc, job) < 0) {
        return bg_error(spec, "failed to set background");
    }

    printf_fd(STDOUT_FILENO, "[%d] %s &\n",
              job->job_id,
              job->cmd_line ? job->cmd_line : "");

    return 0;
}