#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "io.h"


static int history_error(const char *msg)
{
	printf_fd(STDERR_FILENO, "history: %s\n", msg);
	return 1;
}



int bi_history(t_exec_ctx *ctx, char **argv, size_t argc)
{
	(void) argv;
	
	if (!ctx || !ctx->state){
		errno = EINVAL;
		return 1;
	}
	
	if (argc > 1)
		return history_error("слишком много аргументов");
	
	t_hist *hist = ctx->state->hist;
	
	if (!hist || hist->size == 0)
		return 0;
	
	size_t start;
	
	if (hist->size < HIST_MAX)
		start = 0;
	else
		start = hist->head;
	
	for (size_t i = 0; i < hist->size; ++i){
		
		size_t idx = (start + i) % HIST_MAX;
		
		if (!hist->lines[idx])
			continue;
		
		printf_fd(STDOUT_FILENO, "%zu %s\n", i + 1, hist->lines[idx]);
	}
	return 0;
}