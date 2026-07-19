#ifndef READLINE_H
#define READLINE_H

#include "history.h"

char *readline_fd(
        int fd,
        const char *prompt,
        t_hist *hist);

#endif