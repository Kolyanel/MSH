#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "expand.h"
#include "redir.h"
#include "io.h"


t_exec_cmd *expand_command(t_exec_state *st, t_cmd *src)
{
    if (!st || !src) {
        errno = EINVAL;
        return NULL;
    }

    printf_fd(STDERR_FILENO, "DEBUG: expand_command: src->redirs.argc=%zu\n", src->redirs.argc);

    t_exec_cmd *dst = exec_cmd_create();

    if (!dst)
        return NULL;

    size_t argc = vec_size(&src->argv);
    t_token **argv = (t_token**)src->argv.val;

    dst->argv = calloc(argc + 1, sizeof(char*));

    if (!dst->argv) {
        exec_cmd_free(dst);
        return NULL;
    }

    for (size_t i = 0; i < argc; ++i) {
        char *arg = expand_word(st, argv[i]);

        if (!arg) {
            exec_cmd_free(dst);
            return NULL;
        }

        dst->argv[i] = arg;
    }

    dst->argv[argc] = NULL;

    size_t rcnt = vec_size(&src->redirs);
    t_redir **reds = (t_redir**)src->redirs.val;

    printf_fd(STDERR_FILENO, "DEBUG: expand_command: rcnt=%zu\n", rcnt);

    for (size_t i = 0; i < rcnt; ++i) {
        printf_fd(STDERR_FILENO, "DEBUG: expand_command: expanding redir %zu\n", i);

        t_exec_redir *er = redir_expand(st, reds[i]);

        if (!er) {
            printf_fd(STDERR_FILENO, "DEBUG: expand_command: redir_expand failed\n");
            exec_cmd_free(dst);
            return NULL;
        }

        if (vec_push(&dst->redirs, er) < 0) {
            exec_redir_free(er);
            exec_cmd_free(dst);
            return NULL;
        }
    }

    printf_fd(STDERR_FILENO, "DEBUG: expand_command: dst->redirs.argc=%zu\n", dst->redirs.argc);

    return dst;
}