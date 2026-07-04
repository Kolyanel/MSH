#ifndef EXEC_PIPE_H
#define EXEC_PIPE_H

#include <stdbool.h>


/*
* абстракция pipe
* (канал между процессами)
*/
typedef struct s_exec_pipe
{
	/* --- fd[0] - чтение, fd[1] - запись --- */
	int fd[2];
	
	/* --- флаг закрыт ли pipe --- */
	bool closed;
	
} t_exec_pipe;


/* --- API pipe --- */

// создать pipe
int exec_pipe_create(t_exec_pipe *p);

// закрыть fd
void exec_pipe_close(t_exec_pipe *p);

// уничтожить объект
void exec_pipe_destroy(void *ptr);

#endif // EXEC_PIPE_H