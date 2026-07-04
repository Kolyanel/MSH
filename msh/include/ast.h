#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "vector.h"
#include "token.h"


/* --- redirect type --- */

typedef enum e_redir_type
{
	REDIR_IN,               // <
	REDIR_OUT,             // >
	REDIR_APPEND,      // >>
	REDIR_HEREDOC    // <<
} t_redir_type;


/* --- command link --- */

typedef enum e_cmd_link
{
	CMD_LINK_NONE,
	CMD_LINK_SEMI,  // ;
	CMD_LINK_AND,    //  &&
	CMD_LINK_OR       //   ||
} t_cmd_link;


/* --- redirect --- */

typedef struct s_redir
{
	t_redir_type type;
	t_token *target;  // файл
} t_redir;


/* --- command --- */

typedef struct s_cmd
{
	Vector argv;
	Vector redirs;
} t_cmd;


/* --- pipeline --- */

typedef struct s_pipeline
{
	Vector commands;
} t_pipeline;


typedef struct s_and_or
{
	Vector pipelines;
	Vector links;
	bool background;
} t_and_or;


/* ----- AST root ----- */

typedef struct s_ast
{
	Vector nodes;
} t_ast;


/* ---- free ---- */

void ast_free(t_ast *ast);

void redir_free(void *ptr);

void cmd_free(void *ptr);

void pipeline_free(void *ptr);

void and_or_free(void *ptr);


#endif // AST_H