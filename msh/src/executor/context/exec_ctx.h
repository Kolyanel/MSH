#ifndef EXEC_CTX_H
#define EXEC_CTX_H

#include <sys/types.h>
#include <stdbool.h>

#include "exec_state.h"
#include "ast.h"
#include "vector.h"
#include "exec_job.h"
#include "exec_result.h"

/* --- контекст выполнения одного pipeline --- */
struct s_exec_ctx;


/*
* стадия выполнения pipeline
*/
typedef int (*t_exec_stage) (struct s_exec_ctx *ctx);




/*
** Модель владения ресурсами:
**
** owned (владеет ctx):
**   - pipes (все созданные pipe-объекты)
**
** borrowed (не владеет):
**   - state (глобальное состояние shell)
**   - pipeline (AST pipeline)
**
** transferred ownership:
**   - job (передаётся в state после сборки)
*/

typedef struct s_exec_ctx
{
/* --- внешние ссыли (не владеет памятью) --- */
	
	t_exec_state *state; // глобальное состояние shell
	
	t_pipeline *pipeline; // AST pipeline
	
/* --- owned ресурсы --- */
	
	/* -- t_exec_pipe * -- */
	Vector pipes; // все pipe между процессами
	
	t_exec_job *job; // текущий job (owned до передачи в state)
	
	t_exec_result *result;
	
	pid_t last_pid;
	
	bool skip_job_ctrl;
	
} t_exec_ctx;


/* -- API контекста -- */

int exec_ctx_init(t_exec_ctx *ctx, t_exec_state *st, t_pipeline *pl, t_exec_result *result);

void exec_ctx_free(t_exec_ctx *ctx);


#endif  //  EXEC_CTX_H