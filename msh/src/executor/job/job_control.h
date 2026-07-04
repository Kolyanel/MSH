#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>
#include <stdbool.h>

#include "vector.h"
#include "exec_job.h"


typedef struct s_job_control
{
    Vector jobs;
    int shell_tty_fd;
    pid_t shell_pgid;
    pid_t foreground_pgid;
    int next_job_id;
    t_exec_job *fg_job;
    bool job_control_enabled;
    pid_t last_bg_pid;
} t_job_control;


int job_ctrl_init(t_job_control *jc);
void job_ctrl_cleanup(t_job_control *jc);

int job_ctrl_add(t_job_control *jc, t_exec_job *job);
int job_ctrl_remove(t_job_control *jc, int job_id);

t_exec_job *job_ctrl_find(t_job_control *jc, int job_id);
t_exec_job *job_ctrl_find_last(t_job_control *jc);
t_exec_job *job_ctrl_find_prev(t_job_control *jc);
t_exec_job *job_ctrl_find_last_stopped(t_job_control *jc);

void job_ctrl_print(t_job_control *jc, int out_fd);

int job_ctrl_set_fg(t_job_control *jc, t_exec_job *job);
int job_ctrl_set_bg(t_job_control *jc, t_exec_job *job);

t_exec_process *job_ctrl_find_process(t_job_control *jc, pid_t pid);
t_exec_job *job_ctrl_find_pgid(t_job_control *jc, pid_t pgid);

t_exec_job *job_lookup(t_job_control *jc, const char *arg);

#endif // JOB_CONTROL_H