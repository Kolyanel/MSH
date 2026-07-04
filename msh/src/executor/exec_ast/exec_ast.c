#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "exec_ast.h"
#include "exec_pipeline.h"
#include "io.h"


t_exec_res exec_ast_run(t_exec_state *st, t_ast *ast, t_exec_result *result)
{
    if (!st || !ast || !result) {
        errno = EINVAL;
        return EXEC_ERR;
    }

    result->exit_requested = false;
    result->exit_code = 0;

    size_t node_cnt = vec_size(&ast->nodes);

    if (node_cnt == 0)
        return EXEC_OK;

    t_and_or **nodes = (t_and_or **)ast->nodes.val;

    for (size_t i = 0; i < node_cnt; ++i) {

        t_and_or *ao = nodes[i];

        if (!ao)
            continue;

        size_t pl_cnt = vec_size(&ao->pipelines);
        t_pipeline **pipelines = (t_pipeline **)ao->pipelines.val;

        size_t link_cnt = vec_size(&ao->links);
        t_cmd_link **links = (t_cmd_link **)ao->links.val;

        if (pl_cnt != link_cnt + 1) {
            errno = EINVAL;
            return EXEC_ERR;
        }

        int last_status = 0;
        bool run_next = true;

        for (size_t j = 0; j < pl_cnt && run_next; ++j) {

            t_pipeline *pl = pipelines[j];

            if (!pl) {
                errno = EINVAL;
                return EXEC_ERR;
            }

            t_pipeline_res pres = {0};

            /* ============================================================
             * exec_pipeline_run(st, pl, ao->background, &pres)
             * ============================================================ */
            if (exec_pipeline_run(st, pl, ao->background, &pres) == EXEC_ERR) {
                return EXEC_ERR;
            }

            st->last_status = pres.exec.exit_code;
            last_status = pres.exec.exit_code;

            if (pres.exec.exit_requested) {
                *result = pres.exec;
                return EXEC_OK;
            }

            if (j < link_cnt) {

                switch (*links[j]) {

                case CMD_LINK_AND:
                    if (last_status != 0)
                        run_next = false;
                    break;

                case CMD_LINK_OR:
                    if (last_status == 0)
                        run_next = false;
                    break;

                default:
                    break;
                }
            }
        }
    }

    result->exit_code = st->last_status;

    return EXEC_OK;
}