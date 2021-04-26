
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include "sbmem.h"
#include <sys/stat.h>

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
//char* name = "/ShMem";
//#define SHMEM_NAME "/ShMem"

// Define semaphore(s)
//#define SEM_NAME "/sem"

// Define your stuctures and variables.

sem_t *sem;
void * addr1;

struct block{
    pid_t pid;
    void *start_pos;
    int length;
    int is_allocated;
    int next;
};

struct block *block_list;
off_t fsize;


void divideBlock(struct block *pBlock, int n, int index);
int find_buddy(int index);
int min(int x, int y);
int max(int x, int y);

int sbmem_remove()
{
    shm_unlink(SHMEM_NAME);
    sem_unlink(SEM_NAME);
    return (0);
}

int sbmem_init(int segmentsize)
{
    int fd;
    printf ("sbmem init called\n");
    int n = ceil(log(segmentsize) / log(2)) ;
    int size= (int) pow(2, n)+ (sizeof (struct block))*100;

    fd = shm_open(SHMEM_NAME, O_RDWR | O_EXCL | O_CREAT , 0777);
    if (fd == -1) {
        if (errno == EEXIST){
            printf("File exists :recreating...\n");
            shm_unlink(SHMEM_NAME);
            fd = shm_open(SHMEM_NAME, O_RDWR | O_EXCL | O_CREAT , 0777);
            if (fd == -1) {
                fprintf(stderr, "Open failed:%s\n", strerror(errno));
                return (-1);
            }
        }
        else{
            fprintf(stderr, "Open failed:%s\n", strerror(errno));
            return (-1);
        }
    }

    if (ftruncate(fd, size) == -1) {
        fprintf(stderr, "ftruncate : %s\n", strerror(errno));
        return (-1);
    }
    off_t fsize;
    fsize = lseek(fd, 0, SEEK_END);

    printf("segment size : %ld\n",fsize);

    void * addr;
    addr = mmap(0, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == (void *) -1) {
        fprintf(stderr, "mmap failed : %s\n", strerror(errno));
        return (-1);
    }

    pid_t pid = getpid();
    block_list = addr;
    block_list[0].pid = pid;
    block_list[0].start_pos = addr + (sizeof (struct block))*100;

    block_list[0].length = fsize - (sizeof (struct block))*100;
    block_list[0].is_allocated = 0;
    block_list[0].next = NULL;

    //semaphore ----------------------------------------------------------------------
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL  , 0644, 0);
    if (sem == SEM_FAILED)
    {
        printf( "Error : %s\n", strerror(errno ));
        exit(0);
    }
    printf ("semaphores initialized.\n\n");

    sem_post(sem);
    return (0);
}

int sbmem_open()
{
    int fd;
    fd = shm_open(SHMEM_NAME, O_RDWR, 0777);
    if (fd == -1) {
        fprintf(stderr, "Open failed:%s\n", strerror(errno));
        return (-1);
    }
    off_t fsize;
    fsize = lseek(fd, 0, SEEK_END);

    addr1= mmap(0, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    block_list = addr1;
    block_list[0].start_pos = (sizeof (struct block))*100;

    //semaphore-------------------------------------------------------------------------
    sem = sem_open (SEM_NAME, 0644);
    if (sem == SEM_FAILED)
    {
        printf( "Error : %s\n", strerror(errno ));
        exit(0);
    }

    return (0);
}

void *sbmem_alloc (int size)
{
    sem_wait(sem);
    int pid = getpid();
    void * ptr;

    printf("\nsbmem_alloc is called, pid: %d, size : %d\n", pid, size);
    int power = ceil(log(size) / log(2));
    int n = (int) pow(2, power);

    struct block *curr = block_list;
    int i = 0;
    while ((curr[i].length<n || ((curr[i].is_allocated)==1)) && (curr[i].next!=NULL)){
        printf("One block is checked\n");
        i = curr[i].next;
    }
    if((curr[i].length)==n){
        curr[i].is_allocated = 1;
        curr[i].pid= pid;
        ptr = curr[i].start_pos;
        ptr = (void*)((int)ptr + addr1);

        printf("\nExact fitting block allocated\n\n");

        struct block *tmp = block_list;
        int j = 0;
        while(tmp[j].next!=NULL){
            printf("starting pos: %d, length: %d, is_allocated: %d, next starting pos: %d \n",(int)tmp[j].start_pos, tmp[j].length, tmp[j].is_allocated, (int)tmp[tmp[j].next].start_pos);
            j=tmp[j].next;
        }
        printf("starting pos: %u, length: %d, is_allocated: %d \n",tmp[j].start_pos, tmp[j].length, tmp[j].is_allocated);

        sem_post(sem);
        return (void*)ptr;
    }
    else if((curr[i].length)>(n)){
        divideBlock(curr,n, i);
        curr[i].pid= pid;
        curr[i].is_allocated = 1;
        ptr = curr[i].start_pos;
        ptr = (void*)((int)ptr + addr1);

        printf("\nblock is allocated by dividing\n\n");

        struct block *tmp = block_list;
        int j = 0;
        while(tmp[j].next!=NULL){
            printf("starting pos: %u, length: %d, is_allocated: %d, next starting pos: %u \n",tmp[j].start_pos, tmp[j].length, tmp[j].is_allocated, tmp[tmp[j].next].start_pos);
            j=tmp[j].next;
        }
        printf("starting pos: %u, length: %d, is_allocated: %d \n",tmp[j].start_pos, tmp[j].length, tmp[j].is_allocated);

        sem_post(sem);
        return (void*)ptr;
    }
    else{
        printf("No enough memory to allocate\n\n");
        return NULL;
    }
}

void divideBlock(struct block *pBlock, int n, int index) {
    int temp = pBlock[index].length;
    int i = n;
    pBlock[index].length = n;
    while(i < temp) {
        int j = 0;
        while(pBlock[j].start_pos!=NULL){
            j++;
        }
        pBlock[j].start_pos = (pBlock[index].start_pos) + pBlock[index].length;
        pBlock[j].length = i;
        pBlock[j].is_allocated = 0;
        pBlock[j].next = pBlock[index].next;
        pBlock[index].next = j;
        index = j;
        i= i*2;
    }
}

void sbmem_free (void *p)
{
    if(p == NULL){
        printf("Memory does not exist to free\n");
        exit(1);

    }
    p = (void*)((int)p - (int)addr1);
    sem_wait(sem);
    int pid = getpid();
    struct block *curr = block_list;
    int i = 0;
    while(curr[i].start_pos!=NULL){
        if(curr[i].start_pos == p){
            printf("\nsbmem_free is called.Memory is deallocated, pid: %d, size : %d\n\n", pid, curr[i].length);
            curr[i].is_allocated = 0;
            i = find_buddy(i);
            break;
        }
        i=curr[i].next;
    }
    struct block *tmp = block_list;
    int j = 0;
    while(tmp[j].next!=NULL){
        printf("starting pos: %u, length: %d, is_allocated: %d, next starting pos: %u \n",tmp[j].start_pos, tmp[j].length, tmp[j].is_allocated, tmp[tmp[j].next].start_pos);
        j=tmp[j].next;
    }
    printf("starting pos: %u, length: %d, is_allocated: %d \n",tmp[j].start_pos, tmp[j].length, tmp[j].is_allocated);        sem_post(sem);
    sem_post(sem);
}

int sbmem_close()
{
    munmap(addr1, fsize);
    return (0);
}

int find_buddy(int index){
    int buddy_nr;
    struct block *curr = block_list;

    buddy_nr = (((int)(curr[index].start_pos)-(3200))/(curr[index].length));
    void* buddy_addr;
    if(buddy_nr%2 == 0){
        buddy_addr = (void*)((int)(curr[index].start_pos) + (curr[index].length));
    }
    else{
        buddy_addr = (void*)((int)(curr[index].start_pos) - (curr[index].length));
    }

    int k = 0;
    while(curr[k].start_pos!=NULL){
        if(curr[k].start_pos == buddy_addr && curr[k].is_allocated == 0 && curr[k].length == curr[index].length){
            if(curr[max(index,k)].next != NULL){
                curr[min(index,k)].next = curr[max(index,k)].next;
            }
            else{
                curr[min(index,k)].length += curr[max(index,k)].length;
                curr[max(index,k)].start_pos=NULL;
                curr[min(index,k)].next=NULL;
                break;
            }
            curr[min(index,k)].length += curr[max(index,k)].length;
            curr[max(index,k)].start_pos=NULL;
            index = find_buddy(min(index,k));
            break;
        }
        else if(curr[k].start_pos == buddy_addr && curr[k].is_allocated == 1){
            break;
        }

        if(curr[k].next == NULL){
            break;
        }
        k=curr[k].next;

    }
    return min(index,k);
}

int min(int x, int y)
{
    if(x<y){
        return x;
    }
    else
        return y;
}

int max(int x, int y)
{
    if(x<y){
        return y;
    }
    else
        return x;
}