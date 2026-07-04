#include <stdlib.h>
#include <errno.h>

#include "expand.h"
#include "heredoc.h"
#include "io.h"


t_exec_redir *redir_expand(t_exec_state *st, t_redir *r)
{
    if (!st || !r) {
        errno = EINVAL;
        return NULL;
    }

    char *targ = expand_word(st, r->target);
    if (!targ)
        return NULL;

    t_exec_redir *er = exec_redir_create(r->type, targ);
    if (!er) {
        free(targ);
        return NULL;
    }

    if (er->type == REDIR_HEREDOC) {
        if (collect_heredoc(er) < 0) {
            exec_redir_free(er);
            return NULL;
        }
    }

    return er;
}