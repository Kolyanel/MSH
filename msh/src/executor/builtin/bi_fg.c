#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "job_control.h"
#include "exec_wait.h"
#include "io.h"


static int fg_error(const char *spec, const char *msg)
{
    if (spec) {
        printf_fd(STDERR_FILENO, "fg: %s: %s\n", spec, msg);
    } else {
        printf_fd(STDERR_FILENO, "fg: %s\n", msg);
    }
    return 1;
}


int bi_fg(t_exec_ctx *ctx, char **argv, size_t argc)
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
            puts_fd(STDERR_FILENO, "fg: no current job\n", 0);
            return 1;
        }
    } else {
        job = job_lookup(jc, spec);
        if (!job) {
            return fg_error(spec, "no such job");
        }
    }

    if (job->state == JOB_DONE) {
        return fg_error(spec, "job already done");
    }

    if (job->cmd_line) {
        printf_fd(STDOUT_FILENO, "%s\n", job->cmd_line);
    }

    /* Отправляем SIGCONT, чтобы продолжить остановленный процесс */
    if (job->pgid > 0) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            return fg_error(spec, "failed to continue job");
        }
    }

    /* Отдаём терминал job'у */
    if (job_ctrl_set_fg(jc, job) < 0) {
        return fg_error(spec, "failed to set foreground");
    }

    /* Ждём завершения job */
    int status = wait_foreground(jc, job);

    if (status < 0) {
        return fg_error(spec, "wait failed");
    }

    job->exit_code = exec_wait_status_to_exit_code(status);

    return job->exit_code;
}