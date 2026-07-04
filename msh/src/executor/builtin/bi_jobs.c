#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "job_control.h"
#include "io.h"


static int jobs_error(const char *msg)
{
	printf_fd(STDERR_FILENO, "jobs: %s\n", msg);
	
	return 1;
}



int bi_jobs(t_exec_ctx *ctx, char **argv, size_t argc)
{
	(void) argv;
	
	if (!ctx || !ctx->state){
		errno = EINVAL;
		return 1;
	}
	
	if (argc != 1)
		return jobs_error("to many arguments");
	
	job_ctrl_print(&ctx->state->job_ctrl, STDOUT_FILENO);
	
	return 0;
}