#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>

#include "prompt.h"
#include "term_color.h"
#include "env.h"


/* - внутрений helper - */

static int appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...)
{
	if (!buf || !len || !cap || !fmt)
		return -1;
	
	va_list ap;
	va_start(ap, fmt);
	
	int need = vsnprintf(NULL, 0, fmt, ap);
	
	va_end(ap);
	
	if (need < 0)
		return -1;
	
	if ((size_t) need > SIZE_MAX - *len - 1)
		return -1;
	
	size_t req = *len + (size_t) need + 1;
	
	if (req > *cap){
		
		size_t new_cap = *cap ? *cap * 2 : 64;
		
		while (new_cap < req){
			
			if (new_cap > SIZE_MAX / 2)
				return -1;
			
			new_cap *= 2;
		}
		char *tmp = realloc(*buf, new_cap);
		
		if (!tmp)
			return -1;
		
		*buf = tmp;
		*cap = new_cap;
	}
	
	if (*buf && *len == 0)
		(*buf)[0] = '\0';
	
	va_start(ap, fmt);
	
	int written = vsnprintf(*buf + *len, *cap - *len, fmt, ap);
	va_end(ap);
	
	if (written < 0)
		return -1;
	
	*len += (size_t) written;
	
	return 0;
}



// создание динамического промпта

char *sh_build_prompt(t_shell *sh)
{
	if (!sh){
		errno = EINVAL;
		return NULL;
	}
	
	const char *pwd = env_get(&sh->env, "PWD");
	
	const char *home = env_get(&sh->env, "HOME");
	
	const char *display = pwd ? pwd : "~";
	
	char *short_pwd = NULL;
	
	if  (pwd && home && *home){
		
		size_t home_len = strlen(home);
		
		if (strncmp(pwd, home, home_len) == 0){
			
			if (pwd[home_len] == '\0')
				short_pwd = strdup("~");
			
			else if (pwd[home_len] == '/'){
				
				size_t short_len = strlen(pwd) - home_len + 2;
				short_pwd = malloc(short_len);
				
				if (!short_pwd)
					return NULL;
				
				snprintf(short_pwd, short_len, "~%s", pwd + home_len);
			}
			if (short_pwd)
				display = short_pwd;
		}
	}
	
	char *buf = NULL;
	size_t len = 0;
	size_t cap = 0;
	
//	if (appendf(&buf, &len, &cap, "msh: %s: >> ", display) < 0){
	
	if (appendf(&buf, &len, &cap, "%s%smsh: %s%s%s: >> ", C_BOLD, C_YELLOW, C_GREEN, display, C_RESET) < 0){
		free(short_pwd);
		free(buf);
		return NULL;
	}
	
	/*if(appendf(&buf, &len, &cap, "%s%s", C_BOLD, C_YELLOW) < 0)
		goto err;
		
	if (appendf(&buf, &len, &cap, "msh: ") < 0)
		goto err;
	
	if (appendf(&buf, &len, &cap, "%s", C_GREEN) < 0)
		goto err;
	
	if (appendf(&buf, &len, &cap, "%s", cwd) < 0)
		goto err;
	
	if (appendf(&buf, &len, &cap, "%s", C_RESET) < 0)
		goto err;
	
	if (appendf(&buf, &len, &cap, ": >> ") < 0)
		goto err;*/
	
	free(short_pwd);
	return buf;
}