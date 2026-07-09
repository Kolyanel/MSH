#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "msh.h"
#include "msh_error.h"
#include "shell_loop.h"
#include "shell_script.h"
#include "shell_command.h"
#include "exec_state.h"
#include "env.h"
#include "path.h"
#include "job_control.h"


/* Инициализация shell с собственным вектором окружения */

int shell_init(t_shell *sh, int argc, char **argv, char **envp)
{
    if (!sh)
        return MS_SET_ERR(EINVAL);

    /* Обнуляем всю структуру */
    *sh = (t_shell){0};

    sh->stdin_fd  = -1;
    sh->stdout_fd = -1;
    sh->stderr_fd = -1;
    sh->tty_fd = -1;

    /* Инициализация векторов */
    if (vec_init(&sh->env, 0) < 0){
        msh_perr_fd(STDERR_FILENO, "init env");
        goto cleanup;
    }

    if (vec_init(&sh->path, 0) < 0){
        msh_perr_fd(STDERR_FILENO, "init path");
        goto cleanup;
    }

    /* Сохраняем аргументы */
    sh->sh_argv = argv;
    sh->sh_argc = argc;

    /* Определяем режим работы */
    if (argc >= 3 && argv[1] && strcmp(argv[1], "-c") == 0)
        sh->mode = MSH_MODE_COMMAND;
    else if (argc > 1)
        sh->mode = MSH_MODE_SCRIPT;
    else
        sh->mode = MSH_MODE_INTERACTIVE;

    sh->should_exit = false;

    /* Дублируем файловые дескрипторы */
    sh->stdin_fd  = dup(STDIN_FILENO);
    sh->stdout_fd = dup(STDOUT_FILENO);
    sh->stderr_fd = dup(STDERR_FILENO);

    sh->tty_fd = open("/dev/tty", O_RDWR);

    if (sh->tty_fd < 0){
        msh_perr_fd(STDERR_FILENO, "open /dev/tty");
        sh->tty_fd = dup(STDERR_FILENO);

        if (sh->tty_fd < 0){
            msh_perr_fd(STDERR_FILENO, "dup stderr -> tty");
        }
    }

    if (sh->stdin_fd < 0 || sh->stdout_fd < 0 || sh->stderr_fd < 0 || sh->tty_fd < 0){
        msh_perr_fd(STDERR_FILENO, "init std fd");
        goto cleanup;
    }

    /* История */

    if (hist_init(&sh->hist) < 0){
        msh_perr_fd(STDERR_FILENO, "init history");
        goto cleanup;
    }

    hist_load(&sh->hist, "/root/msh_history");

    /* Окружение */

    if (env_from_envp(&sh->env, envp) < 0){
        msh_perr_fd(STDERR_FILENO, "set env");
        goto cleanup;
    }

    /* PATH */

    if (path_from_env(&sh->path, &sh->env) < 0){
        msh_perr_fd(STDERR_FILENO, "set path");
        goto cleanup;
    }

    sh->exec_state = exec_state_create();

    if (!sh->exec_state){
        msh_perr_fd(STDERR_FILENO, "init exec_state");
        goto cleanup;
    }

    sh->exec_state->env = &sh->env;

    sh->exec_state->path = &sh->path;

    sh->exec_state->hist = &sh->hist;

    /* настройка job control */
    sh->exec_state->job_ctrl.shell_tty_fd = sh->tty_fd;
    sh->exec_state->job_ctrl.shell_pgid = getpgid(0);
    sh->exec_state->job_ctrl.job_control_enabled = true;

    return MS_OK;

cleanup:
    /* Освобождаем всё, что успели выделить */

    shell_cleanup(sh);
    return MS_ERR;
}




/* Полная очистка ресурсов shell */

void shell_cleanup(t_shell *sh)
{
    if (!sh)
        return;

    hist_save(&sh->hist, "/root/msh_history");

    hist_free(&sh->hist);

    if (sh->stdin_fd >= 0) {
        close(sh->stdin_fd);
        sh->stdin_fd = -1;
    }

    if (sh->stdout_fd >= 0) {
        close(sh->stdout_fd);
        sh->stdout_fd = -1;
    }

    if (sh->stderr_fd >= 0) {
        close(sh->stderr_fd);
        sh->stderr_fd = -1;
    }

    if (sh->tty_fd >= 0){
        close(sh->tty_fd);
        sh->tty_fd = -1;
    }

    exec_state_free(sh->exec_state);
    sh->exec_state = NULL;

    vec_free(&sh->path, free);
    vec_free(&sh->env, free_env);

    /* Освобождаем строки, которые могли быть выделены позже */

    free(sh->cur_line);
    sh->cur_line = NULL;

    /* Сбрасываем аргументы (не освобождаем — они из main) */

    sh->sh_argv = NULL;
    sh->sh_argc = 0;

    /* Обнуляем режим и флаги */

    sh->mode = MSH_MODE_INTERACTIVE;
    sh->should_exit = false;
}




/* Единая точка запуска shell в установленном режиме */

int shell_run(t_shell *sh)
{
    if (!sh)
        return MS_SET_ERR(EINVAL);

    switch (sh->mode) {
        case MSH_MODE_INTERACTIVE:
            return shell_loop(sh);

        case MSH_MODE_SCRIPT:
            return shell_script(sh);

        case MSH_MODE_COMMAND:
            return shell_command(sh);

        default:
            return MS_SET_ERR(EINVAL);
    }
}