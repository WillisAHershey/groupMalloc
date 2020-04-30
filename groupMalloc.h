//Willis Hershey wrote this whole damned thing and it is broken and beautiful someone please give him a job
//Last updated February 20th, 2020
#ifndef GROUPMALLOC
#define GROUPMALLOC

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdint.h>

//Comment this next line out before assembling if you don't want thread safety
#define THREAD_SAFE

#ifdef THREAD_SAFE
#	ifdef __STDC_NO_THREADS__
#		ifdef _MSC_VER
#			error Non <thread.h> implementations of mutex are not supported in Windows by this library
#		else
#			include <pthread.h>
#			define mtx_t pthread_mutex_t
#			define WAIT(c) pthread_mutex_lock(c)
#			define POST(c) pthread_mutex_unlock(c)
#			define INIT(c) pthread_mutex_init(c,NULL)
#			define DEST(c) pthread_mutex_destroy(c)
#		endif
#	else
#		include <threads.h>
#		define WAIT(c) mtx_lock(c)
#		define POST(c) mtx_unlock(c)
#		define INIT(c) mtx_init(c,mtx_plain)
#		define DEST(c) mtx_destroy(c)
#	endif
#else
#	define WAIT(c) {}
#	define POST(c) {}
#	define INIT(c) {}
#	define DEST(c) {}
#endif

#ifdef _MSC_VER
#	define PAGESIZE 4096
#	define MMAP_PAGESIZE(c) malloc(c*PAGESZIE)
#	define MUNMAP_PAGESIZE(c,d) free(c)
#else
#	include <unistd.h>
#	include <sys/mman.h>
#	define PAGESIZE sysconf(_SC_PAGE_SIZE)
#	define MMAP_PAGESIZE(c) mmap(NULL,c*PAGESIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED,-1,0)
#	define MUNMAP_PAGESIZE(c,d) munmap(c,d*PAGESIZE)
#	ifndef MAP_UNINITIALIZED
#		define MAP_UNINITIALIZED 0
#	endif
#endif

#define MAPTYPE unsigned long
#define MAPBITS (sizeof(MAPTYPE)*8)

typedef struct groupMallocSlab{
  struct groupMallocSlab *next;
  MAPTYPE maps[0];
}groupMallocSlab_t;

typedef struct groupMalloc{
  struct groupMallocSlab *start;
  struct groupMallocSlab *current;
  size_t objSize;
  size_t objAlign;
  size_t numObjs;
#ifdef THREAD_SAFE
  mtx_t mutex;
#endif
}groupMalloc_t;

#define groupMallocInit(a,b) actualgroupMallocInit(a,sizeof(b),alignof(b))

void actualgroupMallocInit(groupMalloc_t*,size_t,size_t); //Initializes a groupMalloc_t structure at the given address for objects of given size
void* groupMalloc(groupMalloc_t*); //Returns a pointer to an available space in memory that can fit the object
void groupFree(groupMalloc_t*,void*); //Frees the space to the groupMalloc_t structure so that it can be repurposed on a future groupMalloc() call
void groupMallocDestroy(groupMalloc_t*); //Frees all memory associated with the groupMalloc_t structure. Access to any data stored there will be lost

#endif
