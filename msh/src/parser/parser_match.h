#ifndef PARSER_MATCH_H
#define PARSER_MATCH_H


#include "parser.h"
#include "token.h"


/*
* проверяет тип текущего токена
* позицию парсера НЕ двигает
*
* return:
* true  -> текущий токен совпадает
* false -> не совпадает или токена нет
*/

bool parser_check(t_parser *p, t_tok_type type);


/*
* проверяет текущий токен
* если тип совпал:
*     двигает парсер вперед
*     возвращает true
*
* если не совпал:
*     ничего не делает
*     возвращает false
*/

bool parser_match(t_parser *p, t_tok_type type);


/*
* ожидает токен указанного типа
*
* если текущий токен совпадает:
*     двигает парсер вперед
*     возвращает PARSE_OK
*
* если токен не совпадает:
*     возвращает PARSE_NO_MATCH
*
* если произошла внутренняя ошибка:
*     возвращает PARSE_ERR
*     errno установлен
*/

t_parse_res parser_expect(t_parser *p, t_tok_type type);

#endif  // PARSER_MATCH_H