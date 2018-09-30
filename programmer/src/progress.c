
#include <stdio.h>

static void print_progress_bar(int progress)
{
    putchar('[');
    int len = (progress * 78) / 100;
    int i;
    for (i = 0; i < len; i++)
        putchar('#');

    for (; i < 78; i++)
        putchar(' ');

    putchar(']');
}

void progress_start(void)
{
    print_progress_bar(0);
}

void progress_update(int progress)
{
    putchar('\r');
    print_progress_bar(progress);
}

void progress_finish(void)
{
    putchar('\n');
}

