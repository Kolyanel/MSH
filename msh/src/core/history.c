#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "history.h"
#include "msh_error.h"
#include "io.h"


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




/*
* сохранить историю в файл
*/
int hist_save(t_hist *hist, const char *filename)
{
	if (!hist || !filename)
		return MS_SET_ERR(EINVAL);
	
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	
	if (fd < 0)
		return MS_ERR;
	
	size_t start = (hist->size < HIST_MAX) ? 0 : hist->head;
	
	for (size_t i = 0; i < hist->size; ++i){
		
		size_t idx = (start + i) % HIST_MAX;
		
		if (!hist->lines[idx])
			continue;
		
		if (puts_fd(fd, hist->lines[idx], 0) < 0 || puts_fd(fd, "\n", 1) < 0){
			close(fd);
			return MS_ERR;
		}
	}
	close(fd);
	return MS_OK;
}




/*
* загрузить историю из файла
*/
int hist_load(t_hist *hist, const char *filename)
{
	if (!hist || !filename)
		return MS_SET_ERR(EINVAL);
	
	int fd = open(filename, O_RDONLY);
	
	if (fd < 0)
		return MS_OK;
	
	size_t size;
	
	char *data = read_fd(fd, &size);
	
	close(fd);
	
	if (!data)
		return MS_OK;
	
	char *save = data;
	char *line;
	char *rest = data;
	
	while(*rest){
		
		line = rest;
		
		while(*rest && *rest != '\n')
			rest++;
		
		if (*rest == '\n'){
			*rest = '\0';
			rest++;
		}
		
		if (*line)
			hist_push(hist, line);
	}
	
	free(save);
	return MS_OK;
}