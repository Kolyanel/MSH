#include <sys/wait.h>

#include "exec_job.h"


bool exec_is_exited(int ws)
{
	return WIFEXITED(ws);
}


int exec_exit_code(int ws)
{
	if (WIFEXITED(ws))
		return WEXITSTATUS(ws);
	
	return -1;
}


bool exec_is_signaled(int ws)
{
	return WIFSIGNALED(ws);
}


int exec_term_signal(int ws)
{
	if (WIFSIGNALED(ws))
		return WTERMSIG(ws);
	
	return 0;
}


bool exec_is_stopped(int ws)
{
	return WIFSTOPPED(ws);
}


int exec_stop_signal(int ws)
{
	if (WIFSTOPPED(ws))
		return WSTOPSIG(ws);
	
	return 0;
}


bool exec_is_continued(int ws)
{
	#ifdef WIFCONTINUED
		return WIFCONTINUED(ws);
	
	#else
		(void) ws;
		return false;
	#endif
}


int exec_wait_status_to_exit_code(int ws)
{
	if (WIFEXITED(ws))
		return WEXITSTATUS(ws);
	
	if (WIFSIGNALED(ws))
		return 128 + WTERMSIG(ws);
	
	if (WIFSTOPPED(ws))
		return 128 + WSTOPSIG(ws);
	
	return 1;
}