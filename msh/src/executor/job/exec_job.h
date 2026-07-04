#ifndef EXEC_JOB_H
#define EXEC_JOB_H

#include <sys/types.h>
#include <time.h>
#include <stdbool.h>

#include "vector.h"


typedef enum e_exec_kind
{
	EXEC_EXTERNAL,
	EXEC_BUILTIN
} t_exec_kind;

typedef enum e_job_state
{
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE
} t_job_state;


typedef struct s_builtin t_builtin;


/* -- процесс (одна команда после fork) -- */

typedef struct s_exec_process
{
	pid_t pid; // PID процесса
	
	int wait_status;
	
	t_exec_kind kind;
	
	const t_builtin *builtin;
	
/* --- стандартные потоки --- */
	
	int stdin_fd; // ввод
	
	int stdout_fd; // вывод
	
	int stderr_fd; // ошибки
	
	char *exec_path;
	
	char **argv;
	  
/* --- t_exec_redir --- */
	Vector redirs;
	
	bool stopped;
	bool completed;
	bool continued;
	
	int term_signal;
	
	int stop_signal;
	
} t_exec_process;


/* --- JOB набор процессов одного pipeline --- */

typedef struct s_exec_job
{
	/* --- t_exec_process * --- */
	Vector processes; // список процессов
	
	pid_t pgid; // группа процессов
	
	bool foreground; // выполняется в фоне или нет
	
/* --- состояние job --- */
	bool started;
	bool notified;
	
	char *cmd_line;
	
	int job_id;
	
	time_t start_time;
	
	int exit_code;
	
	t_job_state state;
	
} t_exec_job;


/* --- process --- */
t_exec_process *exec_process_create(void);

void exec_process_destroy(void *ptr);


/* ---- job ---- */
t_exec_job *exec_job_create(void);

void exec_job_destroy(void *ptr);



bool exec_is_exited(int ws);
int exec_exit_code(int ws);

bool exec_is_signaled(int ws);
int exec_term_signal(int ws);


bool exec_is_stopped(int ws);
int exec_stop_signal(int ws);


bool exec_is_continued(int ws);

int exec_wait_status_to_exit_code(int ws);


#endif  //  EXEC_JOB_H