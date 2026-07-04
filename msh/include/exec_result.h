#ifndef EXEC_RESULT_H
#define EXEC_RESULT_H

#include <sys/types.h>
#include <stdbool.h>


/* --- результат выполнения команд --- */
typedef enum e_exec_res
{
	EXEC_OK = 0,
	EXEC_ERR = -1
} t_exec_res;


/* --- runtime результат исполнения pipeline/command --- */
typedef struct s_exec_result
{
	bool exit_requested;  // exit builtin запросил завершение shell
	
	int exit_code; // финальный код выхода shell (0 - 255)
	
} t_exec_result;


typedef struct s_pipeline_res
{
	t_exec_result exec;
	
	pid_t pgid;
	pid_t last_pid;
} t_pipeline_res;

#endif // EXEC_RESULT_H