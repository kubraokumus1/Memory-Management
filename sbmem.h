#ifndef SBMEM_H
#define SBMEM_H

#define SHMEM_NAME "/ShMem"

// Define semaphore(s)
#define SEM_NAME "/sem"

int sbmem_init(int segmentsize); 
int sbmem_remove(); 

int sbmem_open(); 
void *sbmem_alloc (int size);
void sbmem_free(void *p);
int sbmem_close(); 

#endif

    
    
