#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "msh.h"


int main(int argc, char *argv[], char **envp)
{
	
	setlocale(LC_ALL, "");

    // создаем и инициализируем структуру всего проекта
    t_shell sh = {0};
    int status;
    
    if (shell_init(&sh, argc, argv, envp) < 0) {
        perror("msh: init");
        shell_cleanup(&sh);
        return EXIT_FAILURE;
    }
    
    status = shell_run(&sh);

    shell_cleanup(&sh);

    return status;
}