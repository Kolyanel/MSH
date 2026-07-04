#ifndef TOKEN_PART_H
#define TOKEN_PART_H

#include <stddef.h>

#include "vector.h"


/* - тип части слова - */

typedef enum e_part_type
{
	PART_TEXT, //текст
	PART_VAR, // переменная после $
	PART_SQ, //литерал
	PART_DQ //кавычки
} t_part_type;


/* --- часть слова --- */

typedef struct s_word_part
{
	t_part_type type;
	char *val;
} t_word_part;


/* создание/удаление */

t_word_part *part_new(t_part_type type, char *val);

void part_free(void *ptr);


t_word_part *part_text(char *val);

t_word_part *part_var(char *val);

t_word_part *part_sq(char *val);

t_word_part *part_dq(char *val);


/*
* тип функций создания частей слова
*/

typedef t_word_part* (*t_part_builder)(char*);



/*
* добавляет новую часть слова part в Vector parts
*/

int part_append(char *str, Vector *parts, t_part_builder builder);


#endif  //TOKEN_PART_H