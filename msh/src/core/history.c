#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "history.h"



int hist_init(
        t_hist *hist)
{
    if (!hist)
    {
        errno = EINVAL;
        return (-1);
    }


    memset(
            hist,
            0,
            sizeof(*hist));


    return (0);
}



void hist_free(
        t_hist *hist)
{
    size_t i;


    if (!hist)
        return;


    i = 0;

    while (i < HIST_MAX)
    {
        free(hist->lines[i]);
        i++;
    }


    memset(
            hist,
            0,
            sizeof(*hist));
}



int hist_push(
        t_hist *hist,
        const char *str)
{
    char *copy;
    size_t last;


    if (!hist || !str || !*str)
    {
        errno = EINVAL;
        return (-1);
    }



    if (hist->size > 0)
    {
        last = (hist->head + HIST_MAX - 1)
                % HIST_MAX;


        if (hist->lines[last]
            && strcmp(hist->lines[last], str) == 0)
        {
            return (0);
        }
    }



    copy = strdup(str);

    if (!copy)
    {
        errno = ENOMEM;
        return (-1);
    }



    free(hist->lines[hist->head]);


    hist->lines[hist->head] = copy;


    hist->head =
        (hist->head + 1) % HIST_MAX;


    if (hist->size < HIST_MAX)
        hist->size++;


    hist->index = hist->head;


    return (0);
}



int hist_save(
        t_hist *hist,
        const char *filename)
{
    int fd;
    size_t start;
    size_t i;


    if (!hist || !filename)
    {
        errno = EINVAL;
        return (-1);
    }



    fd = open(
            filename,
            O_WRONLY | O_CREAT | O_TRUNC,
            0600);


    if (fd < 0)
        return (-1);



    start = (hist->size == HIST_MAX)
        ? hist->head
        : 0;



    i = 0;

    while (i < hist->size)
    {
        size_t idx;


        idx = (start + i) % HIST_MAX;


        if (hist->lines[idx])
        {
            if (write(
                    fd,
                    hist->lines[idx],
                    strlen(hist->lines[idx])) < 0)
            {
                close(fd);
                return (-1);
            }


            if (write(
                    fd,
                    "\n",
                    1) < 0)
            {
                close(fd);
                return (-1);
            }
        }


        i++;
    }



    close(fd);


    return (0);
}



int hist_load(
        t_hist *hist,
        const char *filename)
{
    FILE *fp;
    char *line;
    size_t len;


    if (!hist || !filename)
    {
        errno = EINVAL;
        return (-1);
    }



    fp = fopen(
            filename,
            "r");


    if (!fp)
    {
        if (errno == ENOENT)
            return (0);

        return (-1);
    }



    line = NULL;
    len = 0;


    while (getline(&line, &len, fp) >= 0)
    {
        size_t n;


        n = strlen(line);


        if (n > 0 && line[n - 1] == '\n')
            line[n - 1] = '\0';


        hist_push(
                hist,
                line);
    }



    free(line);

    fclose(fp);


    return (0);
}