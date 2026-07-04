#include <stdlib.h>
#include <errno.h>

#include "token_part.h"


t_word_part *part_new(t_part_type type, char *val)
{
	if (!val){
		errno = EINVAL;
		return NULL;
	}
	
	t_word_part *p = malloc(sizeof(*p));
	
	if (!p){
		free(val);
		errno = ENOMEM;
		return NULL;
	}
	
	p->type = type;
	p->val = val;
	
	return p;
}



t_word_part *part_text(char *val)
{
	return part_new(PART_TEXT, val);
}



t_word_part *part_var(char *val)
{
	return part_new(PART_VAR, val);
}



t_word_part *part_sq(char *val)
{
	return part_new(PART_SQ, val);
}



t_word_part *part_dq(char *val)
{
	return part_new(PART_DQ, val);
}



void part_free(void *ptr)
{
	t_word_part *p = (t_word_part*) ptr;
	
	if (!p)
		return;
		
	if (p->val)
		free(p->val);
	free(p);
}




/*
* добавляет новую часть слова в Vector parts
*/

int part_append(char *str, Vector *parts, t_part_builder builder)
{
	if (!str || !parts || !builder){
		errno = EINVAL;
		return -1;
	}
	
	t_word_part *p = builder(str);
	
	if (!p){
		free(str);
		return -1;
	}
	
	if (vec_push(parts, p) < 0){
		part_free(p);
		return -1;
	}
	
	return 0;
}