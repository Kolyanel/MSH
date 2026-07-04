#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "vector.h"
#include "my_string.h"
#include "path_utils.h"


static int split_components(Vector *parts, const char *path)
{
	if (!parts || !path){
		errno = EINVAL;
		return -1;
	}
	
	char **tok = parser_str(path, "/");
	
	if (!tok)
		return -1;
	
	for (size_t i = 0; tok[i]; ++i){
		if (tok[i][0] == '\0')
			continue;
		
		if (strcmp(tok[i], ".") == 0)
			continue;
		
		if (strcmp(tok[i], "..") == 0){
			
			char *s = vec_pop(parts);
			
			if (s)
				free(s);
			
			continue;
		}
		char *dup = strdup(tok[i]);
		
		if (!dup){
			free_args(tok);
			return -1;
		}
		
		if (vec_push(parts, dup) < 0){
			free(dup);
			free_args(tok);
			return -1;
		}
	}
	free_args(tok);
	
	return 0;
}



static char *build_path(Vector *parts)
{
	if (!parts){
		errno = EINVAL;
		return NULL;
	}
	
	size_t total = 1;
	
	size_t parts_cnt = vec_size(parts);
	
	if (parts_cnt == 0)
		return strdup("/");
	
	for (size_t i = 0; i < parts_cnt; ++i){
		char *s = vec_get(parts, i);
		
		if (!s)
			continue;
		
		total += strlen(s) + 1;
	}
	
	char *res = malloc(total + 1);
	
	if (!res){
		errno = ENOMEM;
		return NULL;
	}
	
	char *p = res;
	
	*p++ = '/';
	
	for (size_t i = 0; i < parts_cnt; ++i){
		
		char *s = vec_get(parts, i);
		
		if (!s)
			continue;
		
		size_t len = strlen(s);
		
		memcpy(p, s, len);
		
		p += len;
		
		if (i + 1 < parts_cnt)
			*p++ = '/';
	}
	*p = '\0';
	
	return res;
}



char *path_normalize(const char *pwd, const char *targ)
{
	if (!targ){
		errno = EINVAL;
		return NULL;
	}
	
	Vector parts;
	
	if (vec_init(&parts, 0) < 0)
		return NULL;
	
	if (targ[0] == '/'){
		
		if (split_components(&parts, targ) < 0)
			goto fail;
	}
	else{
		if (!pwd){
			errno = EINVAL;
			goto fail;
		}
		
		if (split_components(&parts, pwd) < 0)
			goto fail;
		
		if (split_components(&parts, targ) < 0)
			goto fail;
	}
	
	char *res = build_path(&parts);
	
	if (!res)
		goto fail;
	
	vec_free(&parts, free);
	
	return res;

fail:
	vec_free(&parts, free);
	return NULL;
}