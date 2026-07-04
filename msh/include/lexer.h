#ifndef LEXER_H
#define LEXER_H

#include "vector.h"


/*
 * Результат работы правил lexer (внутренний контракт)
 *
 * LEX_OK       — правило сработало, вход продвинут
 * LEX_ERR      — ошибка (errno установлен)
 */
 
typedef enum e_lex_status
{
	LEX_ERR = -1,
	LEX_OK = 0,
	LEX_SYNTAX = 1
} t_lex_status;


// lexer

t_lex_status lex_line(const char *input, Vector *tokens);


#endif //LEXER_H