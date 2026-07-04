#ifndef EXEC_PIPELINE_H
#define EXEC_PIPELINE_H

#include <stdbool.h>

#include "ast.h"
#include "exec_state.h"
#include "exec_result.h"


/* -- запуск pipeline -- */
t_exec_res exec_pipeline_run(t_exec_state *st,
                             t_pipeline *pl,
                             bool background,
                             t_pipeline_res *res);


#endif  // EXEC_PIPELINE_H