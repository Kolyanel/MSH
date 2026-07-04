#ifndef EXEC_ARGV_H
#define EXEC_ARGV_H

#include "exec_state.h"
#include "exec_result.h"


int exec_argv_run(t_exec_state *st, char **argv, t_exec_result *res);

#endif // EXEC_ARGV_H