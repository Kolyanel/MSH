#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "exec_pipe.h"


static void close_fd(int *fd)
{
	if (!fd || *fd < 0)
		return;
	
	(void) close(*fd);
	
	*fd = -1;
}



int exec_pipe_create(t_exec_pipe *p)
{
	if (!p){
		errno = EINVAL;
		return -1;
	}
	
	p->fd[0] = -1;
	p->fd[1] = -1;
	
	p->closed = false;
	
	if (pipe(p->fd) < 0){
		p->fd[0] = -1;
		p->fd[1] = -1;
		return -1;
	}
	
	return 0;
}



void exec_pipe_close(t_exec_pipe *p)
{
	if (!p)
		return;
	
	close_fd(&p->fd[0]);
	
	close_fd(&p->fd[1]);
	
	p->closed = true;
}



void exec_pipe_destroy(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_pipe *p = ptr;
	
	exec_pipe_close(p);
	
	free(p);
}