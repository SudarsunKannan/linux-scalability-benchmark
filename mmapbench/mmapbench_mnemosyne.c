/**
 * @file   mmap.c
 * @author Wang Yuanxuan <zellux@gmail.com>
 * @date   Fri Jan  8 21:23:31 2010
 * 
 * @brief  An implementation of mmap bench mentioned in OSMark paper
 * 
 * 
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#include "config.h"
#include "bench.h"

#define USEFILE

#ifdef USEFILE
#define MAP_SCM 0x80000
#define MAP_PERSISTENT MAP_SCM
#define MAP_FLAGS MAP_SHARED |MAP_PERSISTENT
#else
#define MAP_FLAGS MAP_ANONYMOUS | MAP_PRIVATE
#endif

//#define MAP_FLAGS MAP_PRIVATE
#define __NR_nv_mmap_pgoff     301

 int nbufs = 128000; 
//int nbufs = 64000;
char *shared_area = NULL;
int flag[32];
int ncores = 4;
//char *filename = "/mnt/pcmfs/share.dat";
char *filename = "/mnt/pmfs/share.dat";

void *
worker(void *args)
{
    int id = (long) args;
    int ret = 0;
    int i;

    affinity_set(id);

    for (i = 0; i < nbufs; i++)
        ret += shared_area[i *4096];
    //printf("potato_test: thread#%d done.\n", core);
    return (void *) (long) ret;
}


struct nvmap_arg_struct{

        unsigned long fd;
        unsigned long offset;
        int chunk_id;
        int proc_id;
        int pflags;
        int noPersist;
        int refcount;
};



int create_file(char* filename, size_t bytes) {

    int fd = -1;
    int result =0;	
    
    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
                perror("Error opening file for writing");
                exit(EXIT_FAILURE);
    }
    result = lseek(fd, bytes, SEEK_SET);
    if (result == -1) {
	    close(fd);
    	perror("Error calling lseek() to 'stretch' the file");
	    exit(EXIT_FAILURE);
    }
    result = write(fd, "", 1);
    if (result != 1) {
        close(fd);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }
    printf("going to mmap \n");
	close(fd);

	return 0;
}


int
main(int argc, char **argv)
{
    int i, fd;
    pthread_t tid[32];
    uint64_t start, end, usec;
    struct nvmap_arg_struct a;
    unsigned long offset = 0;
    int chunk_id = 120;
    int proc_id = 20;

    for (i = 0; i < ncores; i++) {
        flag[i] = 0;
    }

    if (argc > 1) {
        ncores = atoi(argv[1]);
    }
#ifdef USEFILE
    create_file(filename, (1 + nbufs) * 4096);
    fd = open(filename, O_RDONLY);
    a.fd = fd;
#else
    a.fd = -1;	 
#endif

    a.offset = offset;
    a.chunk_id =chunk_id;
    a.proc_id = proc_id;
    a.pflags = 1;
    a.noPersist = 0;
    shared_area = mmap(0, (1 + nbufs) * 4096, PROT_READ, MAP_FLAGS, fd, 0);
    //shared_area = (char *) syscall(__NR_nv_mmap_pgoff, 0,(1 + nbufs) * 4096,  PROT_READ | PROT_WRITE, MAP_PRIVATE| MAP_ANONYMOUS, &a);
    if (shared_area == MAP_FAILED) {
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
     }   
     printf("map %lu\n", (unsigned long)shared_area);
         
    start = read_tsc();
    for (i = 0; i < ncores; i++) {
        pthread_create(&tid[i], NULL, worker, (void *) (long) i);
    }

    for (i = 0; i < ncores; i++) {
        pthread_join(tid[i], NULL);
    }
    
    end = read_tsc();
    usec = (end - start) * 1000000 / get_cpu_freq();
    printf("usec: %ld\t\n", usec);

    close(fd);
    return 0;
}
