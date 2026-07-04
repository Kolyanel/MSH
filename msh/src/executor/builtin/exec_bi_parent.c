#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "exec_builtin.h"
#include "redir.h"


static void restore_fds(int in, int out, int err)
{
	if (in >= 0){
		(void) dup2(in, STDIN_FILENO);
		close(in);
	}
	
	if (out >= 0){
		(void) dup2(out, STDOUT_FILENO);
		close(out);
	}
	
	if (err >= 0){
		(void) dup2(err, STDERR_FILENO);
		close(err);
	}
}



int exec_builtin_parent(t_exec_ctx *ctx, t_exec_process *pr)
{
	if (!ctx || !pr || !pr->builtin || !pr->argv){
		errno = EINVAL;
		return -1;
	}
	
	/* сохраняем оригинальные fd */
	
	int saved_stdin = dup(STDIN_FILENO);
	
	int saved_stdout = dup(STDOUT_FILENO);
	
	int saved_stderr = dup(STDERR_FILENO);
	
	if (saved_stdin < 0 || saved_stdout < 0 || saved_stderr < 0){
		
		if (saved_stdin >= 0)
			close(saved_stdin);
		
		if (saved_stdout >= 0)
			close(saved_stdout);
		
		if (saved_stderr >= 0)
			close(saved_stderr);
		return -1;
	}
	
	/* применяем redirect */
	
	if (exec_redir_apply(pr) < 0){
		restore_fds(saved_stdin, saved_stdout, saved_stderr);
		return -1;
	}
	
	/* подсчет argc */
	
	size_t argc = 0;
	
	while (pr->argv[argc])
		argc++;
	
	/* выполняем builtin */
	
	int status = builtin_exec(pr->builtin, ctx, pr->argv, argc);
	
	status = builtin_normalize_status(status);
	
	/* restore shell fd */
	
	restore_fds(saved_stdin, saved_stdout, saved_stderr);
	
	return status;
}