#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "handler.h"
#include "lex_string.h"
#include "token_part.h"
#include "lex_utils.h"



int handle_text(const char **s, t_token *tok)
{
	if (!s || !*s || !tok){
		errno = EINVAL;
		return -1;
	}
	
	const char *start = *s;
	
	while (**s && !is_word_boundary(**s) && **s != '\\' && **s != '\'' && **s != '"' && **s != '$')
		(*s)++;
	
	size_t len = *s - start;
	
	char *str = strndup(start, len);
	
	if (!str){
		errno = ENOMEM;
		return -1;
	}
	
	if (part_append(str, &tok->parts, part_text) < 0)
		return -1;
	
	return 0;
}



int handle_escape(const char **s, t_token *tok)
{
	if (!s || !*s || !tok){
		errno = EINVAL;
		return -1;
	}
	
	char esc;
	
	if (read_escape(s, &esc) < 0)
		return -1;
	
	char *str = malloc(2);
	
	if (!str){
		errno = ENOMEM;
		return -1;
	}
	
	str[0] = esc;
	str[1] = '\0';
	
	if (part_append(str, &tok->parts, part_text) < 0)
		return -1;
	
	return 0;
}



int handle_variable(const char **s, t_token *tok)
{
	if (!s || !*s || !tok){
		errno = EINVAL;
		return -1;
	}
	
	const char *orig = *s;
	errno = 0;
	
	char *name = read_variable(s);
	
	if (!name){
		if (errno == 0){
			char *str = strdup("$");
			
			if (!str){
				errno = ENOMEM;
				return -1;
			}
			
			if (part_append(str, &tok->parts, part_text) < 0){
				return -1;
			}
			*s = orig + 1;
			return 0;
		}
		return -1;
	}
	
	if (part_append(name, &tok->parts, part_var) < 0){
		return -1;
	}
	return 0;
}



int handle_single_quote(const char **s, t_token *tok)
{
	if (!s || !*s || !tok){
		errno = EINVAL;
		return -1;
	}
	
	if (**s != '\'')
		return 0;
	
	const char *orig = *s;
	
	(*s)++;
	
	const char *start = *s;
	
	while (**s && **s != '\'')
		(*s)++;
		
	if (**s != '\''){
		errno = EILSEQ;
		goto err;
	}
	
	size_t len = *s - start;
	
	char *buf = strndup(start, len);
	
	if (!buf){
		errno = ENOMEM;
		goto err;
	}
	
	if (part_append(buf, &tok->parts, part_sq) < 0)
		goto err;
		
	(*s)++;
	
	return 0;

err:
	*s = orig;
	return -1;
}



int handle_double_quote(const char **s, t_token *tok)
{
	if (!s || !*s || !tok){
		errno = EINVAL;
		return -1;
	}
	
	if (**s != '"')
		return 0;
	
	const char *orig = *s;
	
	(*s)++;
	
	const char *start = *s;
	
	while (**s && **s != '"'){
		
		if (**s == '\\'){
			
			size_t len = *s - start;
			
			if (len > 0){
				
				char *str = strndup(start, len);
				
				if (!str){
					errno = ENOMEM;
					goto err;
				}
				
				if (part_append(str, &tok->parts, part_dq) < 0)
					goto err;
			}
			
			char esc;
			int r = read_escape_dq(s, &esc);
			
			if (r < 0)
				goto err;
			
			if (r == 1){
				char *str = malloc(2);
				if (!str){
					errno = ENOMEM;
					goto err;
				}
				
				str[0] = esc;
				str[1] = '\0';
				
				if (part_append(str, &tok->parts, part_dq) < 0)
					goto err;
			} else{
				
				char *str = strdup("\\");
				if (!str){
					errno = ENOMEM;
					goto err;
				}
				
				if (part_append(str, &tok->parts, part_dq) < 0)
					goto err;
				
				(*s)++;
			}
			start = *s;
			continue;
		}
		
		if (**s == '$'){
			
			size_t len = *s - start;
			
			if (len > 0){
				char *str = strndup(start, len);
				
				if (!str){
					errno = ENOMEM;
					goto err;
				}
				
				if (part_append(str, &tok->parts, part_dq) < 0)
					goto err;
			}
			
			const char *dollar = *s;
			errno = 0;
			char *name = read_variable(s);
			
			if (!name){
				
				if (errno != 0)
					goto err;
				
				char *str = strdup("$");
				if (!str){
					errno = ENOMEM;
					goto err;
				}
				
				if (part_append(str, &tok->parts, part_dq) < 0)
					goto err;
				
				*s = dollar + 1;
				start = *s;
				continue;
			}
				
			if (part_append(name, &tok->parts, part_var) < 0)
				goto err;
				
			start = *s;
			continue;
		}
		
		(*s)++;
	}
	
	if (**s != '"')
		goto err;
	
	size_t len = *s - start;
	
	if (len > 0){
		
		char *str = strndup(start, len);
		if (!str){
			errno = ENOMEM;
			goto err;
		}
		
		if (part_append(str, &tok->parts, part_dq) < 0)
			goto err;
	}
	
	(*s)++;
	
	return 0;
	
err:
	*s = orig;
	return -1;
}



t_word_handler get_handler(char c)
{
	switch (c){
		case '\\':
		return handle_escape;
		
		case '\'':
		return handle_single_quote;
		
		case '"':
		return handle_double_quote;
		
		case '$':
		return handle_variable;
		
		default:
		return handle_text;
	}
}