#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>

#include "exec_pipeline_internal.h"
#include "exec_pipe.h"
#include "msh_debug.h"


void	close_pipes(t_exec_ctx *ctx)
{
	t_exec_pipe	**pipes;
	size_t		cnt;


	if (!ctx || !ctx->pipes.val)
		return;


	pipes = (t_exec_pipe **)ctx->pipes.val;
	cnt = vec_size(&ctx->pipes);


	for (size_t i = 0; i < cnt; i++)
	{
		if (!pipes[i])
			continue;


		if (pipes[i]->fd[0] >= 0)
		{
			close(pipes[i]->fd[0]);
			pipes[i]->fd[0] = -1;
		}


		if (pipes[i]->fd[1] >= 0)
		{
			close(pipes[i]->fd[1]);
			pipes[i]->fd[1] = -1;
		}
	}
}



static int	apply_fd(int fd, int target)
{
	if (fd == target)
		return (0);


	if (dup2(fd, target) < 0)
		return (-1);


	close(fd);

	return (0);
}



static int	apply_process_fds(t_exec_process *pr)
{
	int	devnull;


	if (!pr)
		return (errno = EINVAL, -1);



	/*
	** stdin
	*/
	if (pr->stdin_fd == -1)
	{
		devnull = open("/dev/null", O_RDONLY);

		if (devnull < 0)
			return (-1);


		if (apply_fd(devnull, STDIN_FILENO) < 0)
			return (-1);
	}
	else if (pr->stdin_fd != STDIN_FILENO)
	{
		if (apply_fd(pr->stdin_fd, STDIN_FILENO) < 0)
			return (-1);
	}



	/*
	** stdout
	*/
	if (pr->stdout_fd != STDOUT_FILENO)
	{
		if (apply_fd(pr->stdout_fd, STDOUT_FILENO) < 0)
			return (-1);
	}



	/*
	** stderr
	*/
	if (pr->stderr_fd != STDERR_FILENO)
	{
		if (apply_fd(pr->stderr_fd, STDERR_FILENO) < 0)
			return (-1);
	}


	return (0);
}



int	exec_pl_apply_process(t_exec_ctx *ctx, size_t i)
{
	t_exec_process	*pr;


	assert(ctx);


	if (!ctx
		|| !ctx->job
		|| !ctx->state
		|| !ctx->state->envp)
		return (errno = EINVAL, -1);



	pr = ((t_exec_process **)ctx->job->processes.val)[i];


	if (!pr || !pr->argv || !pr->argv[0])
		return (errno = EINVAL, -1);



	DBG_REDIR(
		"apply pid=%d cmd=%s stdin=%d stdout=%d stderr=%d\n",
		getpid(),
		pr->argv[0],
		pr->stdin_fd,
		pr->stdout_fd,
		pr->stderr_fd);



	if (apply_process_fds(pr) < 0)
		return (-1);



	if (exec_redir_apply(pr) < 0)
		return (-1);



	close_pipes(ctx);



signal(SIGPIPE, SIG_DFL);



	execve(
		pr->exec_path,
		pr->argv,
		ctx->state->envp);



return (-1);
}