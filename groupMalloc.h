#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

//Comment this next line out before assembling if you don't want thread safety
#define THREAD_SAFE

#ifdef THREAD_SAFE
	#include <semaphore.h>
	#define wait(c) sem_wait(c)
	#define post(c) sem_post(c)
#else
	#define wait(c) {}
	#define post(c) {}
#endif

#ifndef _GROUP_MALLOC
#define _GROUP_MALLOC

#ifndef MAP_UNINITIALIZED
//This prevents errors in systems that don't use the MAP_UNINITIALIZED flag, which seems to be most of them
	#define MAP_UNINITIALIZED 0
#endif

#define MAPTYPE unsigned long
#define MAPBITS (sizeof(MAPTYPE)*8)
#define PAGESIZE sysconf(_SC_PAGE_SIZE)
#define MMAP_PAGESIZE(c) mmap(NULL,c*PAGESIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED,-1,0)
#define MUNMAP_PAGESIZE(c,d) munmap(c,d*PAGESIZE)

typedef struct groupMallocSlab{
  struct groupMallocSlab *next;
  MAPTYPE maps[0];
}groupMallocSlab_t;

typedef struct groupMalloc{
  struct groupMallocSlab *start;
  struct groupMallocSlab *current;
  size_t objSize;
  size_t numPages;
  size_t numObjs;
#ifdef THREAD_SAFE
  sem_t turn;
#endif
}groupMalloc_t;

int groupMallocInit(groupMalloc_t*,size_t); //Returns a pointer to a new groupMalloc structure configured for objects of a given size.
void* groupMalloc(groupMalloc_t*); //Returns a pointer to an available space in memory that can fit the object
void groupFree(groupMalloc_t*,void*); //Frees the space to the groupMalloc_t structure so that it can be repurposed on a future groupMalloc() call
void groupMallocDestroy(groupMalloc_t*); //Frees all memory associated with the groupMalloc_t structure. Access to any data stored there will be lost.

#endif
