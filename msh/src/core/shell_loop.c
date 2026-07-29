#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "shell_loop.h"
#include "io.h"
#include "msh_error.h"
#include "prompt.h"
#include "lexer.h"
#include "parser.h"
#include "exec_ast.h"
#include "msh_signal.h"
#include "exec_wait.h"
#include "readline_internal.h"
#include "utf8.h"

int	shell_loop(
		t_shell *sh)
{
	if (!sh)
		return (MS_SET_ERR(EINVAL));

	sh->cur_line = NULL;

	sig_set_state(
		sh->exec_state);

	while (!sh->should_exit)
	{
		Vector		tokens;
		t_ast		ast;
		char		*prompt;
		size_t		prompt_cols;

		memset(&tokens, 0, sizeof(Vector));
		memset(&ast, 0, sizeof(t_ast));

		/*
		** SIGCHLD
		*/
		if (sh->exec_state->signals.sigchld_received)
		{
			sh->exec_state->signals.sigchld_received = 0;
			errno = 0;

			reap_chld(
				&sh->exec_state->job_ctrl);
		}

		/*
		** SIGINT
		*/
		if (sh->exec_state->signals.sigint_received)
		{
			sh->exec_state->signals.sigint_received = 0;
			errno = 0;

			puts_fd(
				sh->tty_fd,
				"\n",
				1);
		}

		/*
		** SIGQUIT
		*/
		if (sh->exec_state->signals.sigquit_received)
		{
			sh->exec_state->signals.sigquit_received = 0;
			errno = 0;
		}

		/*
		** Построить prompt.
		*/
		prompt = sh_build_prompt(sh);

		if (!prompt)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"build_prompt");

			return (MS_ERR);
		}

		/*
		** Prompt содержит ANSI escape sequences.
		**
		** utf8_width_n() учитывает только визуальную ширину.
		*/
		prompt_cols = utf8_width_n(
			prompt,
			strlen(prompt));

		/*
		** Prompt принадлежит shell.
		*/
		if (puts_fd(
				sh->tty_fd,
				prompt,
				strlen(prompt)) < 0)
		{
			free(prompt);

			shell_handle_status(
				sh,
				MS_ERR,
				"write prompt");

			return (MS_ERR);
		}

		free(prompt);

		/*
		** ========================================================
		** readline
		** ========================================================
		**
		** Prompt начинается в текущей позиции терминала.
		**
		** В нормальном интерактивном shell это начало строки:
		**
		**     row = 0
		**     col = visual_width(prompt)
		**
		** Никакого DSR 6n больше нет.
		*/
		errno = 0;

		sh->cur_line = readline_fd(
			sh->tty_fd,
			NULL,
			&sh->hist,
			0,
			prompt_cols);

		/*
		** Ctrl-C.
		*/
		if (!sh->cur_line && errno == EINTR)
		{
			errno = 0;

			puts_fd(
				sh->tty_fd,
				"\r\n",
				2);

			continue;
		}

		/*
		** EOF.
		*/
		if (!sh->cur_line && errno == 0)
		{
			puts_fd(
				sh->tty_fd,
				"\r\n",
				2);

			break;
		}

		/*
		** Ошибка readline.
		*/
		if (!sh->cur_line)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"readline");

			continue;
		}

		errno = 0;

		/*
		** История.
		*/
		hist_push(
			&sh->hist,
			sh->cur_line);

		/*
		** Lexer.
		*/
		if (vec_init(
				&tokens,
				0) < 0)
		{
			shell_handle_status(
				sh,
				MS_ERR,
				"init tokens");

			goto cleanup;
		}

		{
			t_lex_status lex_res;

			lex_res = lex_line(
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
		}

		if (vec_size(&tokens) == 0)
			goto cleanup;

		/*
		** Parser.
		*/
		{
			t_parse_res	parse_res;

			parse_res = parse(
				&tokens,
				&ast);

			if (parse_res == PARSE_ERR)
			{
				shell_handle_status(
					sh,
					MS_ERR,
					"parser");

				goto cleanup;
			}

			if (parse_res == PARSE_NO_MATCH)
			{
				msh_perr_fd(
					sh->stderr_fd,
					"syntax error");

				goto cleanup;
			}
		}

		/*
		** Executor.
		*/
		{
			t_exec_result	ex_result;

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