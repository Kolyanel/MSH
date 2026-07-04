#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "job_control.h"
#include "io.h"
#include "exec_job.h"


int job_ctrl_init(t_job_control *jc)
{
    if (!jc) {
        errno = EINVAL;
        return -1;
    }

    if (vec_init(&jc->jobs, 0) < 0)
        return -1;

    jc->shell_tty_fd = -1;
    jc->shell_pgid = 0;
    jc->foreground_pgid = 0;
    jc->next_job_id = 1;
    jc->fg_job = NULL;
    jc->job_control_enabled = false;
    jc->last_bg_pid = 0;

    return 0;
}


void job_ctrl_cleanup(t_job_control *jc)
{
    if (!jc)
        return;

    vec_free(&jc->jobs, exec_job_destroy);

    jc->fg_job = NULL;
    jc->job_control_enabled = false;
    jc->last_bg_pid = 0;
}


int job_ctrl_add(t_job_control *jc, t_exec_job *job)
{
    if (!jc || !job) {
        errno = EINVAL;
        return -1;
    }

    job->job_id = jc->next_job_id++;

    if (vec_push(&jc->jobs, job) < 0)
        return -1;

    if (job->foreground) {
        jc->fg_job = job;
        jc->foreground_pgid = job->pgid;
    } else {
        jc->last_bg_pid = job->pgid;
    }

    return job->job_id;
}


int job_ctrl_remove(t_job_control *jc, int job_id)
{
    if (!jc) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < jc->jobs.argc; ++i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i];

        if (job && job->job_id == job_id) {
            if (jc->fg_job == job) {
                jc->fg_job = NULL;
                jc->foreground_pgid = 0;
            }

            if (jc->last_bg_pid == job->pgid)
                jc->last_bg_pid = 0;

            vec_remove(&jc->jobs, i, exec_job_destroy);
            return 0;
        }
    }

    errno = ENOENT;
    return -1;
}


t_exec_job *job_ctrl_find(t_job_control *jc, int job_id)
{
    if (!jc) {
        errno = EINVAL;
        return NULL;
    }

    for (size_t i = 0; i < jc->jobs.argc; ++i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i];

        if (job && job->job_id == job_id)
            return job;
    }

    errno = ENOENT;
    return NULL;
}


t_exec_job *job_ctrl_find_last(t_job_control *jc)
{
    if (!jc || jc->jobs.argc == 0) {
        errno = ENOENT;
        return NULL;
    }

    for (size_t i = jc->jobs.argc; i > 0; --i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i - 1];
        if (job && job->state != JOB_DONE) {
            return job;
        }
    }

    errno = ENOENT;
    return NULL;
}


t_exec_job *job_ctrl_find_prev(t_job_control *jc)
{
    if (!jc || jc->jobs.argc < 2) {
        errno = ENOENT;
        return NULL;
    }

    int found = 0;
    for (size_t i = jc->jobs.argc; i > 0; --i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i - 1];
        if (job && job->state != JOB_DONE) {
            if (found) {
                return job;
            }
            found = 1;
        }
    }

    errno = ENOENT;
    return NULL;
}


t_exec_job *job_ctrl_find_last_stopped(t_job_control *jc)
{
    if (!jc || jc->jobs.argc == 0) {
        errno = ENOENT;
        return NULL;
    }

    for (size_t i = jc->jobs.argc; i > 0; --i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i - 1];
        if (job && job->state == JOB_STOPPED) {
            return job;
        }
    }

    errno = ENOENT;
    return NULL;
}


static const char *job_state_to_str(t_job_state state)
{
    switch (state) {
        case JOB_RUNNING: return "Running";
        case JOB_STOPPED: return "Stopped";
        case JOB_DONE:    return "Done";
        default:          return "Unknown";
    }
}


void job_ctrl_print(t_job_control *jc, int out_fd)
{
    if (!jc || out_fd < 0)
        return;

    for (size_t i = 0; i < jc->jobs.argc; ++i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i];

        if (!job)
            continue;

        char sign = (jc->fg_job == job) ? '+' : '-';
        char *cmd = job->cmd_line ? job->cmd_line : "";

        printf_fd(out_fd, "[%d]%c %s\t%s\n",
                  job->job_id, sign,
                  job_state_to_str(job->state), cmd);
    }
}


int job_ctrl_set_fg(t_job_control *jc, t_exec_job *job)
{
    if (!jc || !job) {
        errno = EINVAL;
        return -1;
    }

    if (jc->fg_job == job)
        return 0;

    if (jc->shell_tty_fd >= 0 && job->pgid > 0) {
        if (tcsetpgrp(jc->shell_tty_fd, job->pgid) < 0)
            return -1;
    }

    if (jc->fg_job)
        jc->fg_job->foreground = false;

    job->foreground = true;
    jc->fg_job = job;
    jc->foreground_pgid = job->pgid;

    return 0;
}


int job_ctrl_set_bg(t_job_control *jc, t_exec_job *job)
{
    if (!jc || !job) {
        errno = EINVAL;
        return -1;
    }

    if (jc->fg_job == job) {
        jc->fg_job = NULL;
        jc->foreground_pgid = 0;
    }

    job->foreground = false;
    jc->last_bg_pid = job->pgid;

    return 0;
}


t_exec_process *job_ctrl_find_process(t_job_control *jc, pid_t pid)
{
    if (!jc || pid <= 0) {
        errno = EINVAL;
        return NULL;
    }

    for (size_t i = 0; i < jc->jobs.argc; ++i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i];

        if (!job)
            continue;

        for (size_t j = 0; j < job->processes.argc; ++j) {
            t_exec_process *pr = (t_exec_process*)job->processes.val[j];

            if (pr && pr->pid == pid)
                return pr;
        }
    }

    errno = ENOENT;
    return NULL;
}


t_exec_job *job_ctrl_find_pgid(t_job_control *jc, pid_t pgid)
{
    if (!jc || pgid <= 0) {
        errno = EINVAL;
        return NULL;
    }

    for (size_t i = 0; i < jc->jobs.argc; ++i) {
        t_exec_job *job = (t_exec_job*)jc->jobs.val[i];

        if (job && job->pgid == pgid)
            return job;
    }

    errno = ENOENT;
    return NULL;
}


t_exec_job *job_lookup(t_job_control *jc, const char *arg)
{
    if (!jc || !arg)
        return NULL;

    if (arg[0] == '%')
        arg++;

    if (arg[0] == '\0')
        return job_ctrl_find_last(jc);

    char *endptr;
    long id = strtol(arg, &endptr, 10);

    if (*endptr != '\0' || id <= 0)
        return NULL;

    return job_ctrl_find(jc, (int)id);
}