#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "exec_job.h"
#include "exec_cmd.h"
#include "my_string.h"


t_exec_process *exec_process_create(void)
{
	t_exec_process *pr = calloc(1, sizeof(*pr));
	
	if (!pr){
		errno = ENOMEM;
		return NULL;
	}
	
	if (vec_init(&pr->redirs, 0) < 0){
		free(pr);
		return NULL;
	}
	
	pr->pid = -1;
	
	pr->wait_status = 0;
	
	pr->kind = EXEC_EXTERNAL;
	
	pr->builtin = NULL;
	
	pr->stdin_fd = STDIN_FILENO;
	pr->stdout_fd = STDOUT_FILENO;
	pr->stderr_fd = STDERR_FILENO;
	
	pr->exec_path = NULL;
	pr->argv = NULL;
	
	pr->stopped = false;
	pr->completed = false;
	pr->continued = false;
	
	pr->term_signal = 0;
	pr->stop_signal = 0;
	
	return pr;
}



void exec_process_destroy(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_process *pr = ptr;
	
	free(pr->exec_path);
	
	free_args(pr->argv);
	pr->argv = NULL;
	
	vec_free(&pr->redirs, exec_redir_free);
	
	free(pr);
}



t_exec_job *exec_job_create(void)
{
	t_exec_job *j = calloc(1, sizeof(*j));
	
	if (!j){
		errno = ENOMEM;
		return NULL;
	}
	
	if (vec_init(&j->processes, 0) < 0){
		free(j);
		return NULL;
	}
	
	j->pgid = 0;
	j->job_id = 0;
	
	j->foreground = true;
	
	j->started = false;
	j->notified = false;
	
	j->cmd_line = NULL;
	
	j->start_time = time(NULL);
	
	j->exit_code = 0;
	
	j->state = JOB_RUNNING;
	
	return j;
}




void exec_job_destroy(void *ptr)
{
	if (!ptr)
		return;
	
	t_exec_job *j = ptr;
	
	vec_free(&j->processes, exec_process_destroy);
	
	free(j->cmd_line);
	
	free(j);
}