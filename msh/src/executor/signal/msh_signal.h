#ifndef MSH_SIGNAL_H
#define MSH_SIGNAL_H

#include <signal.h>

struct s_exec_state;



typedef struct s_sig_state
{
	volatile sig_atomic_t sigint_received;
	
	volatile sig_atomic_t sigquit_received;
	
	volatile sig_atomic_t sigchld_received;
	
	volatile sig_atomic_t sigtstp_received;
	
	volatile sig_atomic_t sigcont_received;
	
	
	struct sigaction old_int;
	struct sigaction old_quit;
	struct sigaction old_tstp;
	struct sigaction old_chld;
	struct sigaction old_ttin;
	struct sigaction old_ttou;
	struct sigaction old_cont;
	
} t_sig_state;


int sig_init(t_sig_state *sig);

int sig_set(t_sig_state *sig);

void sig_restore(t_sig_state *sig);

void sig_destroy(t_sig_state *sig);


void sig_set_state(struct s_exec_state *st);


void sig_handle_int(int signo);

void sig_handle_quit(int signo);

void sig_handle_tstp(int signo);

void sig_handle_chld(int signo);

void sig_handle_cont(int signo);


#endif // MSH_SIGNAL_H