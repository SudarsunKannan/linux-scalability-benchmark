/**
 * @file   mmap.c
 * @author Wang Yuanxuan <zellux@gmail.com>
 * @modified by Sudarsun Kannan <sudarsun@gatech.edu>
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

#define PAGESIZE 4096

//#define USEFILE
//#define MNEMOSYNE
#define PVM

#ifdef USEFILE

#define OUTPUT "/mnt/pmfs/share.dat"
//Mnemosyne specific flags
#ifdef MNEMOSYNE
	#define MAP_FLAGS MAP_SHARED |MAP_PERSISTENT
	//#define MAP_FLAGS MAP_PRIVATE |MAP_PERSISTENT
	#define MAP_SCM 0x80000
	#define MAP_PERSISTENT MAP_SCM
#else
	//ramfs or pmfs specific flags
	#define MAP_FLAGS MAP_SHARED 
#endif

char *filename =OUTPUT;
#else //PVM case
	#define __NR_nv_mmap_pgoff     314
	#define MAP_FLAGS MAP_ANONYMOUS | MAP_PRIVATE
	#define DISABLE_PERSIST 0

	struct nvmap_arg_struct{
    	unsigned long fd;
	    unsigned long offset;
    	int chunk_id;
	    int proc_id;
	    int pflags;
	    int nopersist;
    	int refcount;
	};
#endif


//default pages to test
int nbufs = 64000;
char *shared_area = NULL;
int flag[32];
//number of threads
int ncores = 1;

void *
worker(void *args)
{
    int id = (long) args;
    int ret = 0;
    int i;

    affinity_set(id);
    for (i = 0; i < nbufs; i++) {
	    shared_area[i *PAGESIZE] = ret;
        ret += shared_area[i *PAGESIZE];
	}
    return (void *) (long) ret;
}

/*create mmap file for filesystem mode*/
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
	close(fd);
	return 0;
}


int
main(int argc, char **argv)
{
    int i, fd=-1;
    pthread_t tid[32];
    uint64_t start, end, usec;

#ifdef PVM
    struct nvmap_arg_struct a;

	/*PVM needs a chunk and procid*/
    int chunk_id = 120;
    int proc_id = 20;

    unsigned long offset = 0;
#endif

    for (i = 0; i < ncores; i++) {
        flag[i] = 0;
    }

    if (argc > 1) {
        nbufs = atoi(argv[1]);
    }
#ifdef USEFILE
    create_file(filename, (1 + nbufs) * PAGESIZE);
	/*easy to change the flags if we are testing 
	page read performance*/
    fd = open(filename, O_RDWR);
#endif

#ifdef PVM
    a.fd = fd;
    a.offset = offset;
    a.chunk_id =chunk_id;
    a.proc_id = proc_id;
	/*FIXME: What is pflags and persist?*/
    a.pflags = 1;
    a.nopersist=DISABLE_PERSIST;

	//pvm's nvmmap function
    shared_area = (char *) syscall(__NR_nv_mmap_pgoff, 0,(1 + nbufs) * PAGESIZE,  PROT_READ | PROT_WRITE,MAP_FLAGS, &a);
	//shared_area = mmap(0, (1 + nbufs) * PAGESIZE, PROT_READ | PROT_WRITE, MAP_FLAGS, fd, 0);

#else
    shared_area = mmap(0, (1 + nbufs) * PAGESIZE, PROT_READ | PROT_WRITE, MAP_FLAGS, fd, 0);
#endif
    if (shared_area == MAP_FAILED) {
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
     }   

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
