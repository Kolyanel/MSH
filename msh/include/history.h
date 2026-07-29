#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>


#define HIST_MAX 500


typedef struct s_hist
{
	char	*lines[HIST_MAX];

	size_t	size;

	size_t	head;

	size_t	index;

}	t_hist;



int	hist_init(
		t_hist *hist);



void	hist_free(
		t_hist *hist);



int	hist_push(
		t_hist *hist,
		const char *str);



int	hist_save(
		t_hist *hist,
		const char *filename);



int	hist_load(
		t_hist *hist,
		const char *filename);



#endif