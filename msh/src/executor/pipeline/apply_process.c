#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>

#include "exec_pipeline_internal.h"
#include "exec_pipe.h"
#include "msh_debug.h"

void close_pipes(t_exec_ctx *ctx)
{
    if (!ctx || !ctx->pipes.val)
        return;

    t_exec_pipe **pipes = (t_exec_pipe**)ctx->pipes.val;
    size_t cnt = vec_size(&ctx->pipes);

    for (size_t i = 0; i < cnt; ++i)
    {
        if (!pipes[i])
            continue;

        if (pipes[i]->fd[0] >= 0)
            close(pipes[i]->fd[0]);
        if (pipes[i]->fd[1] >= 0)
            close(pipes[i]->fd[1]);

        pipes[i]->fd[0] = -1;
        pipes[i]->fd[1] = -1;
    }
}

int exec_pl_apply_process(t_exec_ctx *ctx, size_t i)
{
    assert(ctx);

    if (!ctx || !ctx->job || !ctx->state || !ctx->state->envp)
        return (errno = EINVAL, -1);

    t_exec_process *pr =
        ((t_exec_process**)ctx->job->processes.val)[i];

    if (!pr || !pr->argv || !pr->argv[0])
        _exit(127);

    DBG_REDIR("apply pid=%d cmd=%s\n", getpid(), pr->argv[0]);

    /* stdin */
    if (pr->stdin_fd == -1)
    {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0)
        {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }
        else
        {
            close(STDIN_FILENO);
        }
    }
    else if (pr->stdin_fd != STDIN_FILENO)
    {
        dup2(pr->stdin_fd, STDIN_FILENO);
    }

    /* stdout */
    if (pr->stdout_fd != STDOUT_FILENO)
        dup2(pr->stdout_fd, STDOUT_FILENO);

    /* stderr */
    if (pr->stderr_fd != STDERR_FILENO)
        dup2(pr->stderr_fd, STDERR_FILENO);

    exec_redir_apply(pr);

    close_pipes(ctx);

    signal(SIGPIPE, SIG_DFL);

    execve(pr->exec_path, pr->argv, ctx->state->envp);

    _exit(errno == ENOENT ? 127 : 126);
}