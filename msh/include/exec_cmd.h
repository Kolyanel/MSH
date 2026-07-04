#ifndef EXEC_CMD_H
#define EXEC_CMD_H

#include "vector.h"
#include "ast.h"


typedef struct s_exec_redir
{
	t_redir_type type;
	char *target;
	int heredoc_fd;
} t_exec_redir;


typedef struct s_exec_cmd
{
	char **argv;
	Vector redirs;
} t_exec_cmd;


typedef struct s_exec_pipeline
{
	t_cmd_link link;
	Vector commands;
} t_exec_pipeline;


t_exec_redir *exec_redir_create(t_redir_type type, char *target);

void exec_redir_free(void *ptr);


t_exec_cmd *exec_cmd_create(void);

void exec_cmd_free(void *ptr);


t_exec_pipeline *exec_pipeline_create(void);

void exec_pipeline_free(void *ptr);

#endif // EXEC_CMD_H