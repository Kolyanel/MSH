#include "exec_pipeline_internal.h"

t_exec_stage g_stage[] =
{
    exec_pl_prepare,
    exec_pl_build_processes,
    exec_pl_resolve,
    exec_pl_wire,
    exec_pl_spawn
};

const size_t g_stage_cnt = sizeof(g_stage) / sizeof(*g_stage);