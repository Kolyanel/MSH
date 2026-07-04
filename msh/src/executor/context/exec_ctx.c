#include <errno.h>

#include "exec_ctx.h"
#include "exec_pipe.h"


int exec_ctx_init(t_exec_ctx *ctx, t_exec_state *st, t_pipeline *pl, t_exec_result *result)
{
    if (!ctx || !st || !pl || !result) {
        errno = EINVAL;
        return -1;
    }

    *ctx = (t_exec_ctx){0};

    ctx->state = st;
    ctx->pipeline = pl;
    ctx->result = result;

    if (vec_init(&ctx->pipes, 0) < 0)
        goto fail;

    return 0;

fail:
    exec_ctx_free(ctx);
    return -1;
}


void exec_ctx_free(t_exec_ctx *ctx)
{
    if (!ctx)
        return;

    vec_free(&ctx->pipes, exec_pipe_destroy);

    /*
     * ============================================================
     * ❗ НЕ УДАЛЯЕМ ctx->job!
     * job принадлежит job_control.
     * Удаление происходит через job_ctrl_remove.
     * ============================================================
     */

    *ctx = (t_exec_ctx){0};
}