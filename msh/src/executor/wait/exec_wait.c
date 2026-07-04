#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include "exec_wait.h"
#include "io.h"
#include "job_control.h"


/* ============================================================
 * FIND JOB BY PID
 * ============================================================ */
static t_exec_job *find_job_by_pid(t_job_control *jc,
                                   pid_t pid,
                                   t_exec_process **out_pr)
{
    assert(jc);
    if (!jc || pid <= 0)
        return NULL;

    for (size_t i = 0; i < jc->jobs.argc; ++i)
    {
        t_exec_job *job = (t_exec_job *)jc->jobs.val[i];
        if (!job)
            continue;

        for (size_t j = 0; j < job->processes.argc; ++j)
        {
            t_exec_process *pr =
                (t_exec_process *)job->processes.val[j];

            if (pr && pr->pid == pid)
            {
                DBG_WAIT("pid=%d matched job_id=%d\n", pid, job->job_id);

                if (out_pr)
                    *out_pr = pr;
                return job;
            }
        }
    }

    return NULL;
}


/* ============================================================
 * UPDATE PROCESS STATUS
 * ============================================================ */
int update_process_status(t_exec_process *pr, int status)
{
    assert(pr);

    if (!pr)
        return (errno = EINVAL, -1);

    pr->wait_status = status;

    if (exec_is_exited(status))
    {
        pr->completed = true;
        pr->stopped = false;
        pr->continued = false;
    }
    else if (exec_is_signaled(status))
    {
        pr->completed = true;
        pr->stopped = false;
        pr->continued = false;
        pr->term_signal = exec_term_signal(status);
    }
    else if (exec_is_stopped(status))
    {
        pr->completed = false;
        pr->stopped = true;
        pr->continued = false;
        pr->stop_signal = exec_stop_signal(status);
    }
    else if (exec_is_continued(status))
    {
        pr->continued = true;
        pr->stopped = false;
    }

    DBG_WAIT("pid=%d status updated: %d\n", pr->pid, status);
    return 0;
}


/* ============================================================
 * UPDATE JOB STATE
 * ============================================================ */
int update_job_state(t_exec_job *job)
{
    assert(job);

    if (!job)
        return (errno = EINVAL, -1);

    bool all_done = true;
    bool all_stopped = true;
    bool has_proc = false;

    t_exec_process *last = NULL;

    if (job->processes.argc > 0)
        last = (t_exec_process *)job->processes.val[job->processes.argc - 1];

    for (size_t i = 0; i < job->processes.argc; ++i)
    {
        t_exec_process *pr =
            (t_exec_process *)job->processes.val[i];

        if (!pr)
            continue;

        has_proc = true;

        if (!pr->completed)
            all_done = false;

        if (!pr->completed && !pr->stopped)
            all_stopped = false;

        pr->continued = false;
    }

    if (!has_proc)
    {
        job->state = JOB_DONE;
        job->exit_code = 0;
        return 0;
    }

    if (last)
        job->exit_code =
            exec_wait_status_to_exit_code(last->wait_status);

    if (all_done)
        job->state = JOB_DONE;
    else if (all_stopped)
        job->state = JOB_STOPPED;
    else
        job->state = JOB_RUNNING;

    DBG_WAIT("job_id=%d state updated=%d exit=%d\n",
             job->job_id, job->state, job->exit_code);

    return 0;
}


/* ============================================================
 * WAIT SINGLE PROCESS
 * ============================================================ */
int wait_process(t_exec_process *pr, int opt)
{
    assert(pr);

    if (!pr || pr->pid <= 0)
        return (errno = EINVAL, -1);

    int status;
    pid_t ret;

    while (1)
    {
        ret = waitpid(pr->pid, &status, opt);

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (ret == 0)
            return 0;

        break;
    }

    return update_process_status(pr, status);
}


/* ============================================================
 * WAIT JOB
 * ============================================================ */
int wait_job(t_exec_job *job, int opt)
{
    assert(job);

    if (!job)
        return (errno = EINVAL, -1);

    if (job->pgid <= 0 || job->processes.argc == 0)
    {
        update_job_state(job);
        return job->exit_code;
    }

    int last_status = 0;

    for (size_t i = 0; i < job->processes.argc; ++i)
    {
        t_exec_process *pr =
            (t_exec_process *)job->processes.val[i];

        if (!pr || pr->pid <= 0)
            continue;

        int status;
        pid_t ret;

        while (1)
        {
            ret = waitpid(pr->pid, &status, opt);

            if (ret < 0 && errno == EINTR)
                continue;

            break;
        }

        update_process_status(pr, status);
        last_status = status;
    }

    update_job_state(job);

    return last_status;
}


/* ============================================================
 * FOREGROUND WAIT
 * ============================================================ */
int wait_foreground(t_job_control *jc, t_exec_job *job)
{
    assert(jc && job);

    if (!jc || !job)
        return (errno = EINVAL, -1);

    DBG_WAIT("FG wait job_id=%d pgid=%d\n", job->job_id, job->pgid);

    jc->fg_job = job;
    jc->foreground_pgid = job->pgid;
    job->foreground = true;

    if (jc->shell_tty_fd >= 0 && job->pgid > 0)
        tcsetpgrp(jc->shell_tty_fd, job->pgid);

    int status = wait_job(job, WUNTRACED | WCONTINUED);

    if (jc->shell_tty_fd >= 0 && jc->shell_pgid > 0)
        tcsetpgrp(jc->shell_tty_fd, jc->shell_pgid);

    job->foreground = false;

    if (jc->fg_job == job)
    {
        jc->fg_job = NULL;
        jc->foreground_pgid = 0;
    }

    DBG_WAIT("FG done job_id=%d status=%d\n",
             job->job_id, status);

    return status;
}


/* ============================================================
 * REAP CHILDREN
 * ============================================================ */
int reap_chld(t_job_control *jc)
{
    assert(jc);

    if (!jc)
        return (errno = EINVAL, -1);

    int status;
    pid_t pid;
    int count = 0;

    while (1)
    {
        pid = waitpid(-1,
                      &status,
                      WNOHANG | WUNTRACED | WCONTINUED);

        if (pid < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == ECHILD)
                break;
            return -1;
        }

        if (pid == 0)
            break;

        t_exec_process *pr = NULL;
        t_exec_job *job =
            find_job_by_pid(jc, pid, &pr);

        if (!job || !pr)
            continue;

        update_process_status(pr, status);
        update_job_state(job);

        if (job->state == JOB_DONE)
        {
            DBG_WAIT("reaping job_id=%d\n", job->job_id);
            job_ctrl_remove(jc, job->job_id);
        }

        count++;
    }

    return count;
}