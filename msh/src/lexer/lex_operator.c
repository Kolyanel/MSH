#include "lexer_rules.h"
#include "token.h"


t_core_res lex_logic(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !*s || !tokens)
		return CORE_ERR;
	
	t_tok_type type;
	const int len = 2;
	
	if (**s == '&' && (*s)[1] && (*s)[1] == '&'){
		type = TOK_AND;
//		len = 2;
	}
	
	else if (**s == '|' && (*s)[1] && (*s)[1] == '|'){
		type = TOK_OR;
//		len = 2;
	}
	
	else
		return CORE_NO_MATCH;
	
	t_token *tok = token_new(type, pos);
	
	if (!tok)
		return CORE_ERR;
	
	if (vec_push(tokens, tok) < 0){
		token_free(tok);
		return CORE_ERR;
	}
	
	*s += len;
	return CORE_OK;
}



t_core_res lex_pipe(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !*s || !tokens)
		return CORE_ERR;
	
	if (**s != '|')
		return CORE_NO_MATCH;
	
	t_token *tok = token_new(TOK_PIPE, pos);
	
	if (!tok)
		return CORE_ERR;
	
	if (vec_push(tokens, tok) < 0){
		token_free(tok);
		return CORE_ERR;
	}
	
	(*s)++;
	
	return CORE_OK;
}




t_core_res lex_background(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !*s || !tokens)
		return CORE_ERR;
	
	if (**s != '&')
		return CORE_NO_MATCH;
	
	t_token *tok = token_new(TOK_BACKGROUND, pos);
	
	if (!tok)
		return CORE_ERR;
	
	if (vec_push(tokens, tok) < 0){
		token_free(tok);
		return CORE_ERR;
	}
	
	(*s)++;
	return CORE_OK;
}



t_core_res lex_redir(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !*s || !tokens)
		return CORE_ERR;
	
	t_tok_type type;
	int len = 1;
	
	if (**s == '<' && (*s)[1] && (*s)[1] == '<'){
		type = TOK_HEREDOC;
		len = 2;
	}
	
	else if (**s == '>' && (*s)[1] && (*s)[1] == '>'){
		type = TOK_REDIR_APPEND;
		len = 2;
	}
	
	else if (**s == '<')
		type = TOK_REDIR_IN;
	
	else if (**s == '>')
		type = TOK_REDIR_OUT;
	
	else
		return CORE_NO_MATCH;
	
	t_token *tok = token_new(type, pos);
	
	if (!tok)
		return CORE_ERR;
	
	if (vec_push(tokens, tok) < 0){
		token_free(tok);
		return CORE_ERR;
	}
	
	*s += len;
	
	return CORE_OK;
}



t_core_res lex_semi(const char **s, Vector *tokens, size_t pos)
{
	if (!s || !*s || !tokens)
		return CORE_ERR;
	
	if (**s != ';')
		return CORE_NO_MATCH;
	
	t_token *tok = token_new(TOK_SEMI, pos);
	
	if (!tok)
		return CORE_ERR;
	
	if (vec_push(tokens, tok) < 0){
		token_free(tok);
		return CORE_ERR;
	}
	
	(*s)++;
	
	return CORE_OK;
}