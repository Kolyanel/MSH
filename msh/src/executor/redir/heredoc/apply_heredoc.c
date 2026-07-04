#include <unistd.h>
#include <errno.h>

#include "heredoc.h"


int apply_heredoc(t_exec_redir *r)
{
	if (!r || r->heredoc_fd < 0){
		errno = EINVAL;
		return -1;
	}
	
	if (dup2(r->heredoc_fd, STDIN_FILENO) < 0)
		return -1;
	
	close(r->heredoc_fd);
	
	r->heredoc_fd = -1;
	
	return 0;
}