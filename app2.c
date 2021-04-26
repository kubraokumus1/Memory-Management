
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "sbmem.h"

int main()
{
    printf("--------------------------------app2------------ \n" );
    int i, ret;
    int pid;

    char *p, *q;
    pid = getpid();
    ret = sbmem_open();
    if (ret == -1)
        exit (1);

    p = sbmem_alloc (64);
    p[0] = 'a';
    printf("p[0]: %c\n", p[0]);
    for (i = 0; i < 64; ++i) {
        p[i] = 'a'; // init all chars to ‘a’
    }

    printf("p[4]: %c\n", p[4]);

    sbmem_free (p);
    sbmem_close();
    return (0);
}
