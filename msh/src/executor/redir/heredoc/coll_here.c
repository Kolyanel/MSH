#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "heredoc.h"
#include "io.h"

static int write_line(int out_fd, const char *line)
{
    if (!line || out_fd < 0)
        return (errno = EINVAL, -1);

    if (puts_fd(out_fd, line, 0) < 0)
        return -1;

    if (puts_fd(out_fd, "\n", 1) < 0)
        return -1;

    return 0;
}

static void close_pipe(int p[2])
{
    close(p[0]);
    close(p[1]);
}

int collect_heredoc(t_exec_redir *r)
{
    if (!r || !r->target)
        return (errno = EINVAL, -1);

    int pipefd[2];

    if (pipe(pipefd) < 0)
        return -1;

    while (1)
    {
        errno = 0;

        if (puts_fd(STDERR_FILENO, "> ", 2) < 0)
        {
            close_pipe(pipefd);
            return -1;
        }

        char *line = read_line(stdin, 0);

        if (!line)
        {
            if (errno != 0)
            {
                close_pipe(pipefd);
                return -1;
            }
            break;
        }

        if (strcmp(line, r->target) == 0)
        {
            free(line);
            break;
        }

        if (write_line(pipefd[1], line) < 0)
        {
            free(line);
            close_pipe(pipefd);
            return -1;
        }

        free(line);
    }

    close(pipefd[1]);

    if (r->heredoc_fd >= 0)
        close(r->heredoc_fd);

    r->heredoc_fd = pipefd[0];
    return 0;
}