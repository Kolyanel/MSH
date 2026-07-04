#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "exec_builtin.h"
#include "exec_ctx.h"
#include "io.h"


static bool is_n_flag(const char *s)
{
	if (!s)
		return false;
	
	if (s[0] != '-' || s[1] == '\0')
		return false;
	
	for (size_t i = 1; s[i]; ++i){
		if (s[i] != 'n')
			return false;
	}
	return true;
}



static size_t parse_flags(char **argv, size_t argc, bool *newline)
{
	if (!argv || !argv[0] || !newline)
		return 0;
	
	size_t i = 1;
	*newline = true;
	
	while (i < argc && is_n_flag(argv[i])){
		*newline = false;
		i++;
	}
	return i;
}



int bi_echo(t_exec_ctx *ctx, char **argv, size_t argc)
{
	if (!ctx || !argv || !argv[0]){
		errno = EINVAL;
		return 1;
	}
	
	bool newline;
	size_t start = parse_flags(argv, argc, &newline);
	
	for (size_t i = start; i < argc; ++i){
		
		puts_fd(STDOUT_FILENO, argv[i], 0);
		
		if (i + 1 < argc)
			puts_fd(STDOUT_FILENO, " ", 1);
	}
	
	if (newline)
		puts_fd(STDOUT_FILENO, "\n", 1);
	
	return 0;
}