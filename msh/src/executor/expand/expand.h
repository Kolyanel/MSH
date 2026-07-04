#ifndef EXPAND_H
#define EXPAND_H

#include "exec_state.h"
#include "ast.h"
#include "exec_cmd.h"


char *expand_var(t_exec_state *st, const char *name);


char *expand_tilde(t_exec_state *st, const char *str);


char *expand_word(t_exec_state *st, t_token *tok);


t_exec_cmd *expand_command(t_exec_state *st, t_cmd *src);


t_exec_pipeline *expand_pipeline(t_exec_state *st, t_pipeline *src);


t_exec_redir *redir_expand(t_exec_state *st, t_redir *r);


#endif // EXPAND_H