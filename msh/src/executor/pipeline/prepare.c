#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "exec_pipeline_internal.h"
#include "exec_pipe.h"
#include "io.h"

int exec_pl_prepare(t_exec_ctx *ctx)
{
    assert(ctx);

    if (!ctx || !ctx->pipeline)
        return (errno = EINVAL, -1);

    size_t cmd_cnt = vec_size(&ctx->pipeline->commands);
    size_t pipe_cnt = (cmd_cnt > 0) ? cmd_cnt - 1 : 0;

    DBG_PIPE("prepare pipes cmd_cnt=%zu pipe_cnt=%zu\n", cmd_cnt, pipe_cnt);

    for (size_t i = 0; i < pipe_cnt; ++i)
    {
        t_exec_pipe *p = calloc(1, sizeof(*p));
        if (!p)
            return (errno = ENOMEM, -1);

        if (exec_pipe_create(p) < 0)
        {
            free(p);
            return -1;
        }

        if (vec_push(&ctx->pipes, p) < 0)
        {
            exec_pipe_destroy(p);
            free(p);
            return -1;
        }
    }

    return 0;
}