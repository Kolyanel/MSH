#ifndef EXEC_WAIT_H
#define EXEC_WAIT_H

#include "exec_job.h"
#include "job_control.h"


int wait_process(t_exec_process *pr, int opt);
int update_process_status(t_exec_process *pr, int wait_status);
int update_job_state(t_exec_job *job);
int wait_job(t_exec_job *job, int opt);
int wait_foreground(t_job_control *jc, t_exec_job *job);
int reap_chld(t_job_control *jc);

#endif // EXEC_WAIT_H