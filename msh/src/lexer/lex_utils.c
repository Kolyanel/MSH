#include <ctype.h>

#include "lex_utils.h"


int is_operator_char(char c)
{
	return (
	c == '|' || 
	c == '&' || 
	c == '<' || 
	c == '>' || 
	c == ';'
	);
}


int is_word_boundary(char c)
{
	return isspace((unsigned char) c) || is_operator_char(c);
}