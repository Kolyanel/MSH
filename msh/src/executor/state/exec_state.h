#ifndef EXEC_STATE_H
#define EXEC_STATE_H


#include <sys/types.h>
#include <stdbool.h>

#include "vector.h"
#include "msh_signal.h"
#include "job_control.h"
#include "history.h"


/* --- глобальное состояние выполнения shell --- */

typedef struct s_exec_state
{
	
	Vector *env;
	
	Vector *path;
	
	char **envp;  // переменные окружения, преобразованые в строки
	
	bool env_dirty;
	bool interactive;
	
	int last_status; // статус последней команды
	
	pid_t shell_pid;
	
	t_sig_state signals;
	
	t_job_control job_ctrl;
	
	t_hist *hist;
	
} t_exec_state;


/* --- init / free --- */

t_exec_state *exec_state_create(void);


void exec_state_free(void *ptr);


int state_update_envp(t_exec_state *st);


int state_update_path(t_exec_state *st);


int state_sync_env(t_exec_state *st);


#endif // EXEC_STATE_H