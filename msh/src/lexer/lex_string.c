#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "lex_string.h"


int read_escape(const char **s, char *out)
{
	if (!s || !*s || !out){
		errno = EINVAL;
		return -1;
	}
	
	if (**s != '\\')
		return -1;
	
	const char *orig = *s;
	
	(*s)++;
	
	if (!**s){
		*s = orig;
		errno = EINVAL;
		return -1;
	}
	
	*out = **s;
	
	(*s)++;
	
	return 0;
}



int read_escape_dq(const char **s, char *out)
{
	if (!s || !*s || !out){
		errno = EINVAL;
		return -1;
	}
	
/*	if (**s != '\\')
		return -1;*/
	
	const char *orig = *s;
	
	if (!(*s)[1]){
		*s = orig;
		errno = EILSEQ;
		return -1;
	}
	
	char c = (*s)[1];
	
	if (c == '"' || c == '\\' || c == '$'){
		*out = c;
		*s += 2;
		return 1;
	}
	 
//	(*s)++;
	return 0;
}




char *read_variable(const char **s)
{
	if (!s || !*s){
		errno = EINVAL;
		return NULL;
	}
	
	if (**s != '$')
		return NULL;
	
	const char *orig = *s;
	
	(*s)++;
	
	if (!**s)
		goto err;
	
	if (**s == '?' || **s == '$' || **s == '!' || isdigit((unsigned char) **s)){
		char *tmp = malloc(2);
		
		if (!tmp){
			errno = ENOMEM;
			goto err;
		}
		
		tmp[0] = **s;
		tmp[1] = '\0';
		
		(*s)++;
		return tmp;
	}
	
	const char *start = *s;
	
	while (**s && (isalnum((unsigned char) **s) || **s == '_'))
		(*s)++;
	
	size_t len = *s - start;
	
	if (len == 0){
		*s = orig;
		return NULL;
	}
	
	char *name = malloc(len + 1);
	
	if (!name){
		errno = ENOMEM;
		goto err;
	}
	
	memcpy(name, start, len);
	
	name[len] = '\0';
	
	return name;

err:
	*s = orig;
	return NULL;
}