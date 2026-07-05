#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "msh_signal.h"
#include "exec_state.h"
#include "job_control.h"
#include "io.h"


static struct s_exec_state *g_state = NULL;


void sig_set_state(struct s_exec_state *st)
{
    g_state = st;
}


int sig_init(t_sig_state *sig)
{
    if (!sig) {
        errno = EINVAL;
        return -1;
    }

    sig->sigint_received = 0;
    sig->sigquit_received = 0;
    sig->sigchld_received = 0;
    sig->sigtstp_received = 0;
    sig->sigcont_received = 0;

    sig->old_int = (struct sigaction){0};
    sig->old_quit = (struct sigaction){0};
    sig->old_tstp = (struct sigaction){0};
    sig->old_chld = (struct sigaction){0};
    sig->old_ttin = (struct sigaction){0};
    sig->old_ttou = (struct sigaction){0};
    sig->old_cont = (struct sigaction){0};

    return 0;
}


static int set_handler(int signo, void (*handle)(int), int flags, struct sigaction *old)
{
    struct sigaction sa = {0};

    sa.sa_handler = handle;

    if (sigemptyset(&sa.sa_mask) < 0)
        return -1;

    sa.sa_flags = flags;

    return sigaction(signo, &sa, old);
}


int sig_set(t_sig_state *sig)
{
    if (!sig) {
        errno = EINVAL;
        return -1;
    }

    if (set_handler(SIGINT, sig_handle_int, SA_RESTART, &sig->old_int) < 0)
        return -1;

    if (set_handler(SIGQUIT, sig_handle_quit, SA_RESTART, &sig->old_quit) < 0)
        return -1;

    if (set_handler(SIGTSTP, sig_handle_tstp, 0, &sig->old_tstp) < 0)
        return -1;

    if (set_handler(SIGCHLD, sig_handle_chld, SA_RESTART, &sig->old_chld) < 0)
        return -1;

    if (set_handler(SIGCONT, sig_handle_cont, SA_RESTART, &sig->old_cont) < 0)
        return -1;

    /* Устанавливаем SIGPIPE в SIG_DFL для дочерних процессов */
    signal(SIGPIPE, SIG_DFL);

    struct sigaction ign = {0};

    ign.sa_handler = SIG_IGN;

    if (sigemptyset(&ign.sa_mask) < 0)
        return -1;

    ign.sa_flags = 0;

    if (sigaction(SIGTTIN, &ign, &sig->old_ttin) < 0)
        return -1;

    if (sigaction(SIGTTOU, &ign, &sig->old_ttou) < 0)
        return -1;

    return 0;
}


void sig_restore(t_sig_state *sig)
{
    if (!sig)
        return;

    sigaction(SIGINT, &sig->old_int, NULL);
    sigaction(SIGQUIT, &sig->old_quit, NULL);
    sigaction(SIGTSTP, &sig->old_tstp, NULL);
    sigaction(SIGCHLD, &sig->old_chld, NULL);
    sigaction(SIGTTIN, &sig->old_ttin, NULL);
    sigaction(SIGTTOU, &sig->old_ttou, NULL);
    sigaction(SIGCONT, &sig->old_cont, NULL);

    g_state = NULL;
}


void sig_destroy(t_sig_state *sig)
{
    if (!sig)
        return;

    sig_restore(sig);
}


void sig_handle_int(int signo)
{
    (void)signo;

    if (!g_state)
        return;

    g_state->signals.sigint_received = 1;

    /* отправить SIGINT foreground-группе процессов */
    if (g_state->job_ctrl.fg_job && g_state->job_ctrl.fg_job->pgid > 0)
        kill(-g_state->job_ctrl.fg_job->pgid, SIGINT);
}


void sig_handle_quit(int signo)
{
    (void)signo;

    if (g_state)
        g_state->signals.sigquit_received = 1;
}


void sig_handle_tstp(int signo)
{
    (void)signo;

    if (!g_state)
        return;

    g_state->signals.sigtstp_received = 1;

    if (g_state->job_ctrl.fg_job) {
        t_exec_job *job = g_state->job_ctrl.fg_job;
        if (job->pgid > 0) {
            kill(-job->pgid, SIGTSTP);
        }
    }
}


void sig_handle_chld(int signo)
{
    (void)signo;

    if (g_state)
        g_state->signals.sigchld_received = 1;
}


void sig_handle_cont(int signo)
{
    (void)signo;

    if (g_state)
        g_state->signals.sigcont_received = 1;
}