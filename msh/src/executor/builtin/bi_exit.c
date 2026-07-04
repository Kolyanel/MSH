#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "io.h"


static void exit_print_err(const char *arg, const char *msg)
{
	puts_fd(STDERR_FILENO, "exit: ", sizeof("exit: ") - 1);
	
	if (arg){
		puts_fd(STDERR_FILENO, arg, 0);
		puts_fd(STDERR_FILENO, ": ", 2);
	}
	
	puts_fd(STDERR_FILENO, msg, 0);
	puts_fd(STDERR_FILENO, "\n", 1);
}




static int parse_exit_code(const char *s, int *out)
{
	if (!s || !out)
		return -1;
	
	long val;
	char *end = NULL;
	
	errno = 0;
	val = strtol(s, &end, 10);
	
	if (errno == ERANGE)
		return -1;
	
	if (end == s || *end != '\0'){
		errno = EINVAL;
		return -1;
	}
	
	*out = (int) ((unsigned long) val & 0xff);
	
	return 0;
}


int bi_exit(t_exec_ctx *ctx, char **argv, size_t argc)
{
	
	if (!ctx || !ctx->result || !ctx->state || !argv || !argv[0]){
		errno = EINVAL;
		return 1;
	}
	
	int code;
	
	if (argc == 1){
		
		code = ctx->state->last_status & 0xff;
		ctx->result->exit_requested = true;
		ctx->result->exit_code = code;
		
		return code;
	}
	
	if (parse_exit_code(argv[1], &code) < 0){
		
		exit_print_err(argv[1], "numeric argument required");
			
		ctx->result->exit_requested = true;
		ctx->result->exit_code = 2;
		
		return 2;
	}
		
	if (argc > 2){
			
		exit_print_err(NULL, "too many arguments");
			
		ctx->result->exit_requested = false;
		ctx->result->exit_code = 1;
		return 1;
	}
		
	ctx->result->exit_requested = true;
	ctx->result->exit_code = code;
	return code;
}