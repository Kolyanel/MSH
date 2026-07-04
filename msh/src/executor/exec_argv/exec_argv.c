#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "exec_argv.h"



int exec_argv_run(t_exec_state *st, char **argv, t_exec_result *res)
{
	if (!st || !argv || !argv[0] || !res){
		errno = EINVAL;
		return -1;
	}
	
	pid_t pid = fork();
	
	if (pid < 0)
		return -1;
	
	if (pid == 0){
		execve(argv[0], argv, st->envp);
		_exit(127);
	}
	
	int status;
	
	while (waitpid(pid, &status, 0) < 0){
		
		if (errno == EINTR)
			continue;
		return -1;
	}
	
	if (WIFEXITED(status))
		res->exit_code = WEXITSTATUS(status);
	
	else if (WIFSIGNALED(status))
		res->exit_code = 128 + WTERMSIG(status);
	
	else
		res->exit_code = 1;
	
	return 0;
}