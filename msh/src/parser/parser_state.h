#ifndef PARSER_STATE_H
#define PARSER_STATE_H

#include "parser.h"
#include "token.h"


/* -- инициализация -- */
/*
* инициализация структуры парсера
*/

int parser_init(t_parser *p, Vector *tokens, t_ast *ast);


/* --- state --- */
/*
* проверяет дошел ли парсер до конца токенов
*/

bool parser_at_end(t_parser *p);


/* --- работа с токенами --- */


/*
* возвращает текущий токен
* не двигает позицию
*/

t_token *parser_cur(t_parser *p);

/*
* возвращает предыдущий токен
* не двигает позицию
*/

t_token *parser_prev(t_parser *p);


/*
* возвращает следующий токен
* не двигает позицию
*/

t_token *parser_next(t_parser *p);


/* --- перемещения --- */

/*
* двигает парсер вперед
*/

int parser_advance(t_parser *p);

/*
* двигает парсер назад
*/

int parser_rewind(t_parser *p);

/*
* устанавливает позицию вручную
*/

int parser_set(t_parser *p, size_t pos);

/*
* возвращает индекс токена
*/

size_t parser_pos(t_parser *p);


#endif  // PARSER_STATE_H