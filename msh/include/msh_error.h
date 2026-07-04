#ifndef MSH_ERROR_H
#define MSH_ERROR_H

#include <errno.h>

#include "msh.h"


#define MS_OK 0
#define MS_ERR -1


#define MS_SET_ERR(e) (errno = (e), MS_ERR)


void msh_perr_fd(int out_fd, const char *msg);


// обработка полученых ошибок
void shell_handle_status(t_shell *sh, int status, const char *msg);


#endif  // MSH_ERROR_H