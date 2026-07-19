#include <stdlib.h>
#include <string.h>

#include "readline.h"
#include "readline_internal.h"
#include "utf8.h"


static int rl_set_prompt(
        t_rl *rl,
        const char *prompt)
{
    if (!rl)
        return (-1);

    if (!prompt)
        prompt = "";

    rl->prompt = strdup(prompt);

    if (!rl->prompt)
        return (-1);

    rl->prompt_bytes = strlen(prompt);

    rl->prompt_cols =
            utf8_display_width(prompt);

    return (0);
}


static void rl_destroy(
        t_rl *rl)
{
    if (!rl)
        return;

    rl_restore(rl);

    buf_free(&rl->line);
    buf_free(&rl->saved_line);

    free(rl->prompt);
    rl->prompt = NULL;

    free(rl->suggestion);
    rl->suggestion = NULL;
}


static char *rl_take_line(
        t_rl *rl)
{
    if (!rl)
        return (NULL);

    if (!rl->line.data)
        return strdup("");

    return strdup(
            rl->line.data);
}


char *readline_fd(
        int fd,
        const char *prompt,
        t_hist *hist)
{
    t_rl rl;
    t_rl_event ev;
    char *result;


    if (rl_init(&rl, fd) < 0)
        return (NULL);


    rl.hist = hist;


    if (rl_set_prompt(&rl, prompt) < 0)
    {
        rl_destroy(&rl);
        return (NULL);
    }


    if (rl.hist)
    {
        rl.hist->index =
                rl.hist->head;
    }


    /*
    ** Renderer owns cursor start position.
    */
    rl_redraw(&rl);


    while (rl.running)
    {
        if (rl_read_key(
                &rl,
                &ev) < 0)
        {
            break;
        }


        rl_process_event(
                &rl,
                &ev);


        if (rl.running)
        {
            rl_redraw(&rl);
        }
    }


            /*
    ** Finish current readline line.
    */
    rl_finish_line(&rl);


    /*
    ** Render state is dead after ENTER.
    ** Terminal owns the new cursor position now.
    */
    rl.render.rows = 0;
    rl.render.width = 0;
    rl.render.end_row = 0;
    rl.render.end_col = 0;


    result =
        rl_take_line(&rl);


    rl_destroy(&rl);


    return (result);
}