#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "shell_loop.h"
#include "io.h"
#include "msh_error.h"
#include "prompt.h"
#include "lexer.h"
#include "parser.h"
#include "exec_ast.h"
#include "msh_signal.h"
#include "exec_wait.h"
#include "readline.h"


int	shell_loop(t_shell *sh)
{
	if (!sh)
		return (MS_SET_ERR(EINVAL));

	sh->cur_line = NULL;

	sig_set_state(sh->exec_state);


	while (!sh->should_exit)
	{
		Vector		tokens;
		t_ast		ast;
		char		*prompt;


		memset(&tokens, 0, sizeof(tokens));
		memset(&ast, 0, sizeof(ast));


		if (sh->exec_state->signals.sigchld_received)
		{
			sh->exec_state->signals.sigchld_received = 0;
			errno = 0;
			reap_chld(&sh->exec_state->job_ctrl);
		}


		if (sh->exec_state->signals.sigint_received)
		{
			sh->exec_state->signals.sigint_received = 0;
			errno = 0;
			puts_fd(sh->tty_fd, "\n", 1);
		}


		if (sh->exec_state->signals.sigquit_received)
		{
			sh->exec_state->signals.sigquit_received = 0;
			errno = 0;
		}


		prompt = sh_build_prompt(sh);

		if (!prompt)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"build_prompt");

			return (MS_ERR);
		}


		errno = 0;


		sh->cur_line = readline_fd(
        sh->tty_fd,
        prompt,
        &sh->hist);


		free(prompt);


		if (!sh->cur_line)
		{
			if (errno == 0)
			{
				puts_fd(
					sh->tty_fd,
					"\n",
					1);

				break;
			}


			if (errno == EINTR)
			{
				errno = 0;
				continue;
			}


			shell_handle_status(
				sh,
				MS_ERR,
				"read_line");

			continue;
		}


		errno = 0;

		hist_push(
			&sh->hist,
			sh->cur_line);



		if (vec_init(&tokens, 0) < 0)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"init tokens");

			goto cleanup;
		}


		t_lex_status lex_res =
			lex_line(
				sh->cur_line,
				&tokens);


		if (lex_res == LEX_ERR)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"lexer");

			goto cleanup;
		}


		if (vec_size(&tokens) == 0)
			goto cleanup;



		t_parse_res r =
			parse(
				&tokens,
				&ast);



		if (r == PARSE_ERR)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"parser");

			goto cleanup;
		}


		if (r == PARSE_NO_MATCH)
		{
			msh_perr_fd(
				sh->stderr_fd,
				"syntax error");

			goto cleanup;
		}



		t_exec_result ex_result;


		if (exec_ast_run(
				sh->exec_state,
				&ast,
				&ex_result) == EXEC_ERR)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"executor");

			goto cleanup;
		}



		if (ex_result.exit_requested)
		{
			sh->should_exit = true;
		}


cleanup:

		ast_free(&ast);

		vec_free(
			&tokens,
			token_free);

		free(sh->cur_line);

		sh->cur_line = NULL;
	}


	return (MS_OK);
}