#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "expand.h"
#include "exec_state.h"
#include "env.h"


char *expand_var(t_exec_state *st, const char *name)
{
	if (!st || !name){
		errno = EINVAL;
		return NULL;
	}
	
	char buf[32];
	
/* --- $? --- */
	if (strcmp(name, "?") == 0){
		
		snprintf(buf, sizeof(buf), "%d", st->last_status);
		return strdup(buf);
	}
	
/* --- $$ --- */
	if (strcmp(name, "$") == 0){
		
		snprintf(buf, sizeof(buf), "%d", st->shell_pid);
		return strdup(buf);
	}
	
/* --- $! --- */
	if (strcmp(name, "!") == 0){
		
		snprintf(buf, sizeof(buf), "%d", st->job_ctrl.last_bg_pid);
		return strdup(buf);
	}
	
	/* --- environ --- */
	const char *val = env_get(st->env, name);
	
	if (!val)
		return strdup("");
	return strdup(val);
}