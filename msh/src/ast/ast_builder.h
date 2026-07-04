#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "ast.h"


t_pipeline *pipeline_create(void);


t_cmd *cmd_create(void);


t_redir *redir_create(t_redir_type type, t_token *target);


t_and_or *and_or_create(void);


int and_or_add_pipeline(t_and_or *node, t_pipeline *pl);


int and_or_add_link(t_and_or *node, t_cmd_link op);


int cmd_add_arg(t_cmd *cmd, t_token *arg);


int cmd_add_redir(t_cmd *cmd, t_redir *redir);


int pipeline_add_cmd(t_pipeline *pl, t_cmd *cmd);


int ast_add_node(t_ast *ast, t_and_or *node);

#endif  // AST_BUILDER_H