#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "buf.h"


static int buf_reserve(t_buf *b, size_t need)
{
	if (!b){
		errno = EINVAL;
		return -1;
	}
	
	if (need > SIZE_MAX - b->len){
		errno = ENOMEM;
		return -1;
	}
	
	if (b->len + need <= b->cap)
		return 0;
		
	if (b->cap >= SIZE_MAX / 2){
		errno = ENOMEM;
		return -1;
	}
	
	size_t new_cap = b->cap ? b->cap : BUF_INIT_CAP;
	
	while (b->len + need > new_cap){
		
		if (new_cap > SIZE_MAX / 2){
			errno = ENOMEM;
			return -1;
		}
		
		new_cap *= 2;
	}
		
	if (new_cap >= SIZE_MAX - 1){
		errno = ENOMEM;
		return -1;
	}
		
	char *tmp = realloc(b->data, (new_cap + 1) * sizeof(*b->data));
		
	if (!tmp){
		errno = ENOMEM;
		return -1;
	}
		
	b->data = tmp;
	b->cap = new_cap;
	
	return 0;
}



int buf_init(t_buf *b)
{
	if (!b){
		errno = EINVAL;
		return -1;
	}
	
	b->data = malloc((BUF_INIT_CAP + 1) * sizeof(*b->data));
	
	if (!b->data){
		errno = ENOMEM;
		b->len = 0;
		b->cap = 0;
		return -1;
	}
	
	b->data[0] = '\0';
	b->len = 0;
	b->cap = BUF_INIT_CAP;
	
	return 0;
}




int buf_append(t_buf *b, char c)
{
	if (!b || !b->data || b->len > b->cap){
		errno = EINVAL;
		return -1;
	}
	
	if (buf_reserve(b, b->len + 1) < 0)
		return -1;
	
	b->data[b->len++] = c;
	b->data[b->len]= '\0';
	
	return 0;
}


int buf_append_span(t_buf *b, const char *s, size_t n)
{
	if (!b || !b->data || (!s && n) || b->len > b->cap){
		errno = EINVAL;
		return -1;
	}
	
	if (n > SIZE_MAX - b->len){
		errno = ENOMEM;
		return -1;
	}
	
	size_t need = b->len + n;
	 
	 if (buf_reserve(b, need) < 0)
	 	return -1;
	 
	 memcpy(b->data + b->len, s, n);
	 
	 b->len += n;
	 b->data[b->len] = '\0';
	 
	 return 0;
}


char *buf_finalize(t_buf *b)
{
	if (!b || !b->data){
		errno = EINVAL;
		return NULL;
	}
	
	char *res = b->data;
	
	b->data = NULL;
	b->len = 0;
	b->cap = 0;
	
	return res;
}


void buf_free(t_buf *b)
{
	if (!b)
		return;
	
	free(b->data);
	b->data = NULL;
	b->len = 0;
	b->cap = 0;
}