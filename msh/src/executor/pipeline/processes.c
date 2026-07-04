#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "exec_pipeline_internal.h"
#include "exec_job.h"
#include "vector.h"
#include "expand.h"
#include "exec_cmd.h"
#include "io.h"

static void cleanup_partial(t_exec_ctx *ctx, size_t upto)
{
    t_exec_process **p = (t_exec_process**)ctx->job->processes.val;

    for (size_t i = 0; i < upto; ++i)
    {
        if (p[i])
            exec_process_destroy(p[i]);
    }

    vec_free(&ctx->job->processes, exec_process_destroy);
}

int exec_pl_build_processes(t_exec_ctx *ctx)
{
    assert(ctx);

    if (!ctx || !ctx->job || !ctx->pipeline || !ctx->state)
        return (errno = EINVAL, -1);

    size_t cmd_cnt = vec_size(&ctx->pipeline->commands);
    t_cmd **cmds = (t_cmd**)ctx->pipeline->commands.val;

    DBG("build_processes cmd_cnt=%zu\n", cmd_cnt);

    for (size_t i = 0; i < cmd_cnt; ++i)
    {
        if (!cmds[i])
        {
            errno = EINVAL;
            cleanup_partial(ctx, i);
            return -1;
        }

        t_exec_process *pr = exec_process_create();
        if (!pr)
        {
            cleanup_partial(ctx, i);
            return -1;
        }

        t_exec_cmd *ec = expand_command(ctx->state, cmds[i]);
        if (!ec)
        {
            DBG("expand_command failed i=%zu\n", i);
            exec_process_destroy(pr);
            cleanup_partial(ctx, i);
            return -1;
        }

        DBG("ec argv=%p first=%p\n",
            (void*)ec->argv,
            (void*)(ec->argv ? ec->argv[0] : NULL));

        pr->argv = ec->argv;
        ec->argv = NULL;

        vec_move(&pr->redirs, &ec->redirs);
        exec_cmd_free(ec);

        if (vec_push(&ctx->job->processes, pr) < 0)
        {
            exec_process_destroy(pr);
            cleanup_partial(ctx, i);
            return -1;
        }
    }

    return 0;
}