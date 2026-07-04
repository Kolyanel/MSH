#include <string.h>
#include <stdlib.h>

#include "history.h"
#include "msh_error.h"


// инициализация структуры t_hist, обнуление всех полей

int hist_init(t_hist *hist)
{
	if (!hist)
		return MS_SET_ERR(EINVAL);
	
	memset(hist, 0, sizeof(*hist));
	
// лучше так делать
//	*hist = (t_hist) {0};
	
	return MS_OK;
}




// очистка hist. безопасная для cleanup

void hist_free(t_hist *hist)
{
	if(!hist)
		return;
	
	for(size_t i = 0; i < HIST_MAX; ++i){
		free(hist->lines[i]);
	}
//	*hist = (t_hist) {0};
	memset(hist, 0, sizeof(*hist));
}




/*
* кольцевое добавление команд пользователя
в историю t_hist
 * игнорируются пустые строки и подряд идущие
 одинаковые команды
 * устанавливает errno
 * возвращает:
 	0 - успех
 	-1 - ошибка
*/

int hist_push(t_hist *h, const char *str)
{
	if (!h || !str || !*str){
		errno = EINVAL;
		return -1;
	}
	
	errno = 0;
		
	size_t last = (HIST_MAX + h->head - 1) % HIST_MAX;
	
	if (h->size > 0 && strcmp(h->lines[last], str) == 0)
		return 0;
	
	char *copy = strdup(str);
	if (!copy){
		errno = ENOMEM;
		return -1;
	}
	
	free(h->lines[h->head]);
	
	h->lines[h->head] = copy;
		
	h->head = (h->head + 1) % HIST_MAX;
	
	h->index = h->head;
		
	if (h->size < HIST_MAX)
		h->size++;
	
	return 0;
}