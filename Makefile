groupMalloc.o: groupMalloc.c groupMalloc.h
	gcc -c -Wall -lpthread -O3 groupMalloc.c
