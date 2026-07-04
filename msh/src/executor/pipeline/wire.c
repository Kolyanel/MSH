#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "exec_pipeline_internal.h"
#include "exec_pipe.h"
#include "io.h"

int exec_pl_wire(t_exec_ctx *ctx)
{
    assert(ctx);

    if (!ctx || !ctx->job || !ctx->pipeline)
        return (errno = EINVAL, -1);

    size_t cnt = vec_size(&ctx->pipeline->commands);

    if (vec_size(&ctx->job->processes) != cnt)
        return (errno = EINVAL, -1);

    if (cnt > 1 && vec_size(&ctx->pipes) != cnt - 1)
        return (errno = EINVAL, -1);

    t_exec_process **p = (t_exec_process**)ctx->job->processes.val;
    t_exec_pipe **pipes = (t_exec_pipe**)ctx->pipes.val;

    for (size_t i = 0; i < cnt; ++i)
    {
        t_exec_process *pr = p[i];
        if (!pr)
            continue;

        pr->stderr_fd = STDERR_FILENO;

        if (cnt == 1)
        {
            pr->stdin_fd = STDIN_FILENO;
            pr->stdout_fd = STDOUT_FILENO;
            continue;
        }

        if (i == 0)
        {
            pr->stdin_fd = STDIN_FILENO;
            pr->stdout_fd = pipes[0]->fd[1];
        }
        else if (i == cnt - 1)
        {
            pr->stdin_fd = pipes[i - 1]->fd[0];
            pr->stdout_fd = STDOUT_FILENO;
        }
        else
        {
            pr->stdin_fd = pipes[i - 1]->fd[0];
            pr->stdout_fd = pipes[i]->fd[1];
        }

        DBG_PIPE("wire[%zu] in=%d out=%d\n", i, pr->stdin_fd, pr->stdout_fd);
    }

    return 0;
}