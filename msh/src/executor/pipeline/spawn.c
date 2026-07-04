#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include "exec_pipeline_internal.h"
#include "exec_pipe.h"
#include "job_control.h"
#include "exec_wait.h"
#include "io.h"

static void spawn_cleanup(t_exec_process **p, size_t spawned, pid_t pgid);

int exec_pl_spawn(t_exec_ctx *ctx)
{
    assert(ctx);

    if (!ctx || !ctx->job || !ctx->state)
        return (errno = EINVAL, -1);

    t_exec_job *job = ctx->job;
    size_t cnt = vec_size(&job->processes);

    if (!cnt)
        return (errno = EINVAL, -1);

    t_exec_process **p = (t_exec_process**)job->processes.val;

    size_t spawned = 0;
    pid_t pgid = 0;

    job->started = false;
    job->pgid = 0;
    ctx->last_pid = 0;

    for (size_t i = 0; i < cnt; ++i)
    {
        t_exec_process *pr = p[i];

        if (!pr || !pr->argv || !pr->argv[0])
            goto fail;

        /* ✅ BUILTIN FIX: run in parent, no fork */
        if (pr->kind == EXEC_BUILTIN && pr->builtin)
        {
            size_t argc = 0;
            while (pr->argv[argc]) argc++;

            int st = builtin_exec(pr->builtin, ctx, pr->argv, argc);

            ctx->last_pid = -1;
            job->exit_code = builtin_normalize_status(st);

            continue;
        }

        pid_t pid = fork();
        if (pid < 0)
            goto fail;

        if (pid == 0)
        {
            if (pgid == 0)
                pgid = getpid();

            setpgid(0, pgid);

            DBG_SPAWN("child pid=%d pgid=%d cmd=%s\n",
                      getpid(), pgid, pr->argv[0]);

            exec_pl_apply_process(ctx, i);
            _exit(127);
        }

        pr->pid = pid;

        if (pgid == 0)
        {
            pgid = pid;
            job->pgid = pid;
        }

        setpgid(pid, pgid);

        ctx->last_pid = pid;
        spawned++;
    }

    job->started = true;

    DBG_SPAWN("spawn done pgid=%d cnt=%zu\n", pgid, cnt);

    return 0;

fail:
    spawn_cleanup(p, spawned, pgid);
    return -1;
}

static void spawn_cleanup(t_exec_process **p, size_t spawned, pid_t pgid)
{
    if (pgid > 0)
        kill(-pgid, SIGKILL);

    for (size_t i = 0; i < spawned; ++i)
    {
        if (!p[i])
            continue;

        if (p[i]->pid > 0)
            waitpid(p[i]->pid, NULL, 0);

        p[i]->pid = -1;
    }
}