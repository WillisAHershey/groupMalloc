//Willis A. Hershey
#include "groupMalloc.h"

//Initializes a groupMalloc structure to efficiently malloc a given number of bytes, and allocates an appropriate amount of pages for the first slab
//Returns 0 on bad inputs or failure, and returns 1 on success

void actualgroupMallocInit(groupMalloc_t *dest,size_t size,size_t alignment){
  printf("size:%zd alignment:%zd\n",size,alignment);
  size_t objSpace=(PAGESIZE-sizeof(MAPTYPE)-sizeof(void*));
  if(objSpace&(alignment-1)){
	printf("ObjSpace fix\n");
	objSpace+=alignment;
	objSpace&=~(alignment-1);
  }
  printf("Objspace:%zd\n",objSpace);

}

void* groupMalloc(groupMalloc_t *group){
  return NULL;
}

void groupFree(groupMalloc_t *group,void *pt){
  return;
}

void groupMallocDestroy(groupMalloc_t *group){
  return;
}
