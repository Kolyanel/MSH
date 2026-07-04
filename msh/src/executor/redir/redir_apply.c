#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "redir.h"
#include "exec_cmd.h"
#include "heredoc.h"


typedef int (*t_redir_fn)(t_exec_redir *r);

static int apply_in(t_exec_redir *r);
static int apply_out(t_exec_redir *r);
static int apply_append(t_exec_redir *r);

static t_redir_fn g_redir_apply[] = {
    [REDIR_IN] = apply_in,
    [REDIR_OUT] = apply_out,
    [REDIR_APPEND] = apply_append,
    [REDIR_HEREDOC] = apply_heredoc
};

static const size_t g_redir_cnt = sizeof(g_redir_apply) / sizeof(g_redir_apply[0]);


static int apply_one(t_exec_redir *r)
{
    if (!r) {
        errno = EINVAL;
        return -1;
    }

    if (r->type < 0 || (size_t)r->type >= g_redir_cnt) {
        errno = EINVAL;
        return -1;
    }

    if (!g_redir_apply[r->type]) {
        errno = EINVAL;
        return -1;
    }

    return g_redir_apply[r->type](r);
}


int exec_redir_apply(t_exec_process *pr)
{
    if (!pr) {
        errno = EINVAL;
        return -1;
    }

    Vector *redirs = &pr->redirs;
    size_t cnt_redirs = vec_size(redirs);

    if (cnt_redirs == 0)
        return 0;

    if (!redirs->val) {
        errno = EINVAL;
        return -1;
    }

    t_exec_redir **reds = (t_exec_redir**)redirs->val;

    for (size_t i = 0; i < cnt_redirs; ++i) {
        t_exec_redir *red = reds[i];
        if (!red)
            continue;

        if (apply_one(red) < 0)
            return -1;
    }

    return 0;
}


static int apply_in(t_exec_redir *r)
{
    if (!r || !r->target) {
        errno = EINVAL;
        return -1;
    }

    int fd = open(r->target, O_RDONLY);
    if (fd < 0)
        return -1;

    if (dup2(fd, STDIN_FILENO) < 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }

    close(fd);
    return 0;
}


static int apply_out(t_exec_redir *r)
{
    if (!r || !r->target) {
        errno = EINVAL;
        return -1;
    }

    int fd = open(r->target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;

    if (dup2(fd, STDOUT_FILENO) < 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }

    close(fd);
    return 0;
}


static int apply_append(t_exec_redir *r)
{
    if (!r || !r->target) {
        errno = EINVAL;
        return -1;
    }

    int fd = open(r->target, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return -1;

    if (dup2(fd, STDOUT_FILENO) < 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }

    close(fd);
    return 0;
}