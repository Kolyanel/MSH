#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

#define HIST_MAX 500


// структура истории команд
/*
 * head - индекс следующей записи
 * size - количество реально сохранённых команд (<= HIST_MAX)
 * index - позиция навигации (up/down), не участвует в записи
 */
 
	typedef struct s_hist{
		
		//массив строк истории
		char *lines[HIST_MAX];
		
		// текущее количество команд
		size_t size;
		
		// куда писать седующую команду. head = (head + 1) % HIST_MAX;
		size_t head;
		
		// позиция/индекс, для навигации
		size_t index;
		
	} t_hist;
	
	
	// инициализация структуры t_hist, обнуление всех полей

int hist_init(t_hist *hist);



// очистка hist. безопасная для cleanup

void hist_free(t_hist *hist);



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

int hist_push(t_hist *h, const char *str);



/*
 * сохранить историю в файл
 * возвращает 0 при успехе, -1 при ошибке
 */
 int hist_save(t_hist *hist, const char *filename);
 
 
 
 /*
 * загрузить историю из файла
 * возвращает 0 при успехе, -1 при ошибке (нет файла — не ошибка)
 */
 int hist_load(t_hist *hist, const char *filename);
	
#endif  //HISTORY_H