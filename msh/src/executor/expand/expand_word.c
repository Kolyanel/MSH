#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "expand.h"
#include "my_string.h"
#include "token_part.h"


char *expand_word(t_exec_state *st, t_token *tok)
{
	if (!st || !tok){
		errno = EINVAL;
		return NULL;
	}
	
	size_t cnt_parts = vec_size(&tok->parts);
	
	if (cnt_parts == 0)
		return strdup("");
	
	t_word_part **parts = (t_word_part**) tok->parts.val;
	
	char *res = NULL;
	
/*	if (parts[0]->type == PART_VAR)
		res = expand_var(st, parts[0]->val);
	
	else if (parts[0]->type == PART_TEXT)
		res = expand_tilde(st, parts[0]->val);
	
	else
		res = strdup(parts[0]->val);
	
	if (!res)
		return NULL;*/
	
	for (size_t i = 0; i < cnt_parts; ++i){
		
		t_word_part *part = parts[i];
		char *piece = NULL;
		
		if (part->type == PART_VAR)
			piece = expand_var(st, part->val);
		
		else if (part->type == PART_TEXT && i == 0)
			piece = expand_tilde(st, part->val);
		
		else
			piece = strdup(part->val);
		
		if (!piece){
			free(res);
			return NULL;
		}
		
		if (!res)
			res = piece;
		else{
			char *tmp = str_join(res, piece);
			
			free(res);
			free(piece);
			
			if (!tmp)
				return NULL;
			
			res = tmp;
		}
	}
	return res;
}