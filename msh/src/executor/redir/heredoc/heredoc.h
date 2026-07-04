#ifndef HEREDOC_H
#define HEREDOC_H

#include "exec_cmd.h"


int collect_heredoc(t_exec_redir *r);


int apply_heredoc(t_exec_redir *r);


#endif  // HEREDOC_H