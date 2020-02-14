#include "groupMalloc.h"

int groupMallocInit(groupMalloc_t *group,size_t size){
  if(!group||!size)
	return 0;
  size_t numPages,numObjs,numMaps;
  if(size*MAPBITS+sizeof(groupMallocSlab_t)+sizeof(MAPTYPE)<=PAGESIZE){
	for(numMaps=1;size*numMaps*MAPBITS+sizeof(MAPTYPE)*numMaps+sizeof(groupMallocSlab_t)<PAGESIZE;++numMaps)
		;
	if(PAGESIZE-sizeof(groupMallocSlab_t)-(numMaps+1)*sizeof(MAPTYPE)-size*MAPBITS*numMaps>size)
		++numMaps;
	numPages=1;
	numObjs=(PAGESIZE-sizeof(groupMallocSlab_t)-numMaps*sizeof(MAPTYPE))/size;
  }
  else{
	numPages=(size*MAPBITS)/PAGESIZE;
	numPages=numPages;
	if(PAGESIZE*numPages-2*sizeof(MAPTYPE)-sizeof(groupMallocSlab_t)-size*MAPBITS>size)
		numMaps=2;
	else
		numMaps=1;
	numObjs=(numPages*PAGESIZE-numMaps*sizeof(MAPTYPE)-sizeof(groupMallocSlab_t))/size;
  }
  groupMallocSlab_t *slab=MMAP_PAGESIZE(numPages);
  if(slab==MAP_FAILED)
	return 0;
  group->start=group->current=slab;
  group->objSize=size;
  group->numPages=numPages;
  group->numObjs=numObjs;
  slab->next=NULL;
  size_t c;
  for(c=0;c<numObjs/MAPBITS;++c)
	slab->maps[c]=~(MAPTYPE)0;
  if(numObjs%MAPBITS)
	slab->maps[c]=(((MAPTYPE)1)<<numObjs%MAPBITS)-1;
#ifdef THREAD_SAFE
  sem_init(&group->turn,0,1);
#endif
  return 1;
}

void* groupMalloc(groupMalloc_t* group){
  groupMallocSlab_t *slab=group->current,*last=NULL;
  size_t numMaps=group->numObjs%MAPBITS?group->numObjs/MAPBITS+1:group->numObjs/MAPBITS;
  MAPTYPE mask;
  size_t c,d;
  wait(&group->turn);
  for(c=0;c<numMaps;++c)
	if(slab->maps[c])
		for(({mask=(MAPTYPE)1;d=0;});d<MAPBITS;({mask<<=1;++d;}))
			if(mask&slab->maps[c]){
				slab->maps[c]&=~mask;
				post(&group->turn);
				return (void*)(((intptr_t)&slab->maps[numMaps])+group->objSize*(MAPBITS*c+d));
			}
  if(slab==group->start){
	last=slab;
	slab=NULL;
  }
  else
	slab=group->start;
  while(slab){
	for(c=0;c<numMaps;++c)
		if(slab->maps[c])
			for(({mask=(MAPTYPE)1;d=0;});d<MAPBITS;({mask<<=1;++d;}))
				if(mask&slab->maps[c]){
					slab->maps[c]&=~mask;
					post(&group->turn);
					return (void*)(((intptr_t)&slab->maps[numMaps])+group->objSize*(MAPBITS*c+d));
				}
	last=slab;
	slab=slab->next;
  }
  last->next=MMAP_PAGESIZE(group->numPages);
  if(!last->next){
	post(&group->turn);
	return NULL;
  }
  slab=last->next;
  slab->next=NULL;
  d=group->numObjs/MAPBITS;
  for(c=0;c<d;++c)
	slab->maps[c]=~(MAPTYPE)0;
  if(group->numObjs%MAPBITS)
	slab->maps[c]=(((MAPTYPE)1)<<(group->numObjs%MAPBITS))-1;
  --slab->maps[0];
  group->current=slab;
  post(&group->turn);
  return (void*)&slab->maps[numMaps];
}

void groupFree(groupMalloc_t *group,void *pt){
  groupMallocSlab_t *slab=group->start,*last=NULL;
  size_t numMaps=group->numObjs%MAPBITS?group->numObjs/MAPBITS+1:group->numObjs/MAPBITS;
  wait(&group->turn);
  while(slab){
	if((intptr_t)pt>(intptr_t)slab&&(intptr_t)pt<(intptr_t)slab+group->numPages*PAGESIZE){
		intptr_t off=((intptr_t)pt-(intptr_t)slab-(intptr_t)sizeof(groupMallocSlab_t)-(intptr_t)(sizeof(MAPTYPE)*numMaps))/(intptr_t)group->objSize;
		slab->maps[off/(intptr_t)MAPBITS]|=((MAPTYPE)1)<<(off%(intptr_t)MAPBITS);
		if(off/(intptr_t)MAPBITS==numMaps&&slab->maps[off/(intptr_t)MAPBITS]==(((MAPTYPE)1)<<(off%(intptr_t)MAPBITS))-1){
			if(last){
				last->next=slab->next;
				MUNMAP_PAGESIZE(slab,group->numPages);
			}
			else if(slab->next){
				group->start=slab->next;
				if(group->current==slab)
					group->current=slab->next;
				MUNMAP_PAGESIZE(slab,group->numPages);
			}
		}
		return;
	}
	last=slab;
	slab=slab->next;
  }
  post(&group->turn);
  return;
}

void groupMallocDestroy(groupMalloc_t *group){
  groupMallocSlab_t *slab=group->start;
  groupMallocSlab_t *next;
  while(slab){
	next=slab->next;
	MUNMAP_PAGESIZE(slab,group->numPages);
	slab=next;
  }
  return;
}
