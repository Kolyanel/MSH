#include <string.h>
#include <unistd.h>

#include "msh_error.h"
#include "io.h"
#include "exec_state.h"


void msh_perr_fd(int err_fd, const char *msg)
{
/*	if (errno == 0)
		return;*/
	
	int err = errno;
	
	(void) puts_fd(err_fd, "msh: ", sizeof("msh: ") - 1);
	
	if (msg)
		(void) puts_fd(err_fd, msg, 0);
	
	if (err != 0){
		(void) puts_fd(err_fd, ": ", sizeof(": ") - 1);
		(void) puts_fd(err_fd, strerror(err), 0);
	}
	
	(void) puts_fd(err_fd, "\n", 1);
	
	errno = err;
}


void shell_handle_status(t_shell *sh, int status, const char *msg)
{
	if (!sh)
		return;
	
	if (status == MS_OK)
		return;
	
	if (errno == 0) {
//        fprintf(stderr, "DEBUG: shell_handle_status called with status=%d, msg='%s', but errno=0\n", status, msg);
        return;
    }
	
	int err = errno;
	
	if (sh->exec_state){
		sh->exec_state->last_status = (status == MS_ERR) ? 1 : status;
	}
	
	int err_fd = (sh->stderr_fd >= 0) ? sh->stderr_fd : STDERR_FILENO;
	
	if (err != 0){
		
		msh_perr_fd(err_fd, msg);
		errno = err;
		return;
	}
	
	printf_fd(err_fd, "msh: %s: внутренняя ошибка (код = %d)\n", msg ? msg : "unknown", sh->exec_state ? sh->exec_state->last_status : 1);
	
}