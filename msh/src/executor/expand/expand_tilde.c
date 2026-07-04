#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "expand.h"
#include "env.h"
#include "my_string.h"



static const char *tilde_resolve(t_exec_state *st, const char *s)
{
	
	if (s[0] == '\0')
		return env_get(st->env, "HOME");
	
	if (strcmp(s, "+") == 0)
		return env_get(st->env, "PWD");
	
	if (strcmp(s, "-") == 0)
		return env_get(st->env, "OLDPWD");
	
	struct passwd *pw = getpwnam(s);
	
	if (!pw)
		return NULL;
	
	return pw->pw_dir;
}



char *expand_tilde(t_exec_state *st, const char *str)
{
	if (!st || !str){
		errno = EINVAL;
		return NULL;
	}
	
	if (str[0] != '~')
		return strdup(str);
	
	const char *p = str + 1;
	
	while (*p && *p != '/')
		p++;
	
	size_t len = p - (str + 1);
	
	char user[256];
	
	if (len >= sizeof(user)){
		errno = ENAMETOOLONG;
		return NULL;
	}
	
	memcpy(user, str + 1, len);
	
	user[len] = '\0';
	
	const char *base = tilde_resolve(st, user);
	
	if (!base)
		return strdup(str);
	
	return str_join(base, p);
}