#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "expand.h"
#include "token_part.h"
#include "my_string.h"


char *build_raw_word(t_token *tok)
{
	if (!tok){
		errno = EINVAL;
		return NULL;
	}
	
	size_t cnt = vec_size(&tok->parts);
	
	if (cnt == 0)
		return strdup("");
	
	t_word_part **parts = (t_word_part**) tok->parts.val;
	
	char *res = NULL;
	
	for (size_t i = 0; i < cnt; ++i){
		
		char *piece = strdup(parts[i]->val);
		
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