#ifndef MSH_H
#define MSH_H

#include <stdbool.h>
#include <stddef.h>

#include "history.h"
#include "vector.h"


typedef struct s_exec_state t_exec_state;

typedef enum e_msh_mode
{
	MSH_MODE_INTERACTIVE = 0,
	MSH_MODE_SCRIPT = 1,
	MSH_MODE_COMMAND = 2
} t_msh_mode;
	
	
// рабочая структура всего msh

typedef struct s_shell
{
	// свои переменные окружения процесса
	Vector env;
	
	// хранение разбитых путей
	Vector path;
	
	
	// последний полученый сигнал
	int last_signal;
	
	
	// текущая строка ввода, до парсинга
	char *cur_line;
	
	
	// аргументы запуска shell или текущего режима
	char **sh_argv;
	
	//  количество аргументов
	size_t sh_argc;
	
	
	// режим работы shell
	// MSH_MODE_INTERACTIVE - терминал
	// MSH_MODE_SCRIPT - скрипт
	// MSH_MODE_COMMAND - -c команда
	t_msh_mode mode;
	
	// флаг выхода из main loop (exit)
	bool should_exit;
	
	
	// сохранённые оригинальные fd (для восстановления после редиректов)
	// стандартные
	int stdin_fd;
	int stdout_fd;
	int stderr_fd;
	// дескриптор управляющего терминала (для prompt и UI)
	int tty_fd;
	
	
	// состояние исполнителя
	// используется при выполнении команд и пайпов
	t_exec_state *exec_state;
	
	
	// история команд
	t_hist hist;
	
} t_shell;


/* инициализация shell с использованием собственного вектора окружения */

int shell_init(t_shell *sh, int argc, char **argv, char **envp);


// освобождение выделенных ресурсов

void shell_cleanup(t_shell *sh);



// единая точка запуска shell в установленом режиме

int shell_run(t_shell *sh);


#endif // MSH_H