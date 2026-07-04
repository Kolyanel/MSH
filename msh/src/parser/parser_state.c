#include <errno.h>

#include "parser_state.h"


/*
* инициализация структуры парсера
*/

int parser_init(t_parser *p, Vector *tokens, t_ast *ast)
{
	if (!p || !tokens || !ast){
		errno = EINVAL;
		return -1;
	}
	
	p->tokens = tokens;
	p->i = 0;
	p->ast = ast;
	
	return 0;
}



/*
* проверяет дошел ли парсер до конца токенов
*/

bool parser_at_end(t_parser *p)
{
	if (!p || !p->tokens)
		return true;
		
	return (p->i >= p->tokens->argc);
}




/*
* возвращает текущий токен
* не двигает позицию
*/

t_token *parser_cur(t_parser *p)
{
	if (!p || parser_at_end(p))
		return NULL;
	
	return (t_token*) p->tokens->val[p->i];
}




/*
* возвращает предыдущий токен
* не двигает позицию
*/

t_token *parser_prev(t_parser *p)
{
	if (!p || !p->tokens || p->i == 0)
		return NULL;
	
	return (t_token*) p->tokens->val[p->i - 1];
}




/*
* возвращает следующий токен
* не двигает позицию
*/

t_token *parser_next(t_parser *p)
{
	if (!p || !p->tokens)
		return NULL;
	
	if (parser_at_end(p))
		return NULL;
	
	if (p->i + 1 >= p->tokens->argc)
		return NULL;
	
	return (t_token *) p->tokens->val[p->i + 1];
}




/*
* двигает парсер вперед
*/

int parser_advance(t_parser *p)
{
	if (!p || !p->tokens){
		errno = EINVAL;
		return -1;
	}
	
	if (parser_at_end(p)){
		errno = ERANGE;
		return -1;
	}
	
	p->i++;
	
	return 0;
}




/*
* двигает парсер назад
*/

int parser_rewind(t_parser *p)
{
	if (!p || !p->tokens){
		errno = EINVAL;
		return -1;
	}
	
	if (p->i == 0){
		errno = ERANGE;
		return -1;
	}
	
	p->i--;
	
	return 0;
}




/*
* устанавливает позицию вручную
*/

int parser_set(t_parser *p, size_t pos)
{
	if (!p || !p->tokens){
		errno = EINVAL;
		return -1;
	}
	
	if (pos > p->tokens->argc){
		errno = ERANGE;
		return -1;
	}
	
	p->i = pos;
	
	return 0;
}




/*
* возвращает индекс токена
*/

size_t parser_pos(t_parser *p)
{
	if (!p)
		return 0;
	
	return p->i;
}