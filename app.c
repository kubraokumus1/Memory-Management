
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "sbmem.h"

#define ASIZE 256
int main()
{
    int i, ret;
    char *p, *q;
    ret = sbmem_open();
    if (ret == -1)
        exit (1);
    p = sbmem_alloc (ASIZE); // allocate space to for characters
    for (i = 0; i < ASIZE; ++i)
        p[i] = 'a'; // init all chars to ‘a’
    sbmem_free (p);

    p = sbmem_alloc (2048);
    sbmem_free(p);
    q = sbmem_alloc (64);

    sbmem_free(q);
    sbmem_close();

    return (0);
}


