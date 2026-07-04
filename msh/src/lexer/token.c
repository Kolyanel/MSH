#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "token.h"
#include "token_part.h"


t_token *token_new(t_tok_type type, size_t pos)
{
	
	t_token *tok = malloc(sizeof(*tok));
	
	if (!tok)
		return NULL;
	
	*tok = (t_token) {0};
	
	if (vec_init(&tok->parts, 0) < 0){
		free(tok);
		return NULL;
	}
	
	tok->type = type;
	tok->pos = pos;
	
	return tok;
}




void token_free(void *ptr)
{
	t_token *tok = (t_token*) ptr;
	
	if (!tok)
		return; 
	
	vec_free(&tok->parts, part_free);
	free(tok->lexeme);
	free(tok);
}




/*
* собирает слово из частей
*/

char *token_to_str(t_token *t)
{
	if (!t || t->type != TOK_WORD){
		errno = EINVAL;
		return NULL;
	}
	
	size_t total = 0;
	
	for (size_t i = 0; i < t->parts.argc; ++i){
		
		t_word_part *p = t->parts.val[i];
		
		if (p && p->val)
			total += strlen(p->val);
	}
	
	char *res = malloc(total + 1);
	
	if (!res){
		errno = ENOMEM;
		return NULL;
	}
	
	char *dst = res;
	
	for (size_t i = 0; i < t->parts.argc; ++i){
		t_word_part *p = t->parts.val[i];
		
		if (p && p->val){
			size_t len = strlen(p->val);
			memcpy(dst, p->val, len);
			dst += len;
		}
	}
	*dst = '\0';
	return res;
}