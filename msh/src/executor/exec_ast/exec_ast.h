#ifndef EXEC_AST_H
#define EXEC_AST_H

#include "ast.h"
#include "exec_state.h"
#include "exec_result.h"


t_exec_res exec_ast_run(t_exec_state *st, t_ast *ast, t_exec_result *result);

#endif // EXEC_AST_H