/* This is a sbrk fix for MUSL... */
/* Use with caution! This might cause issue */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define HEAP_SIZE 0x8000

unsigned char* sbrk_where = NULL;
intptr_t sbrk_increment = 0;

void* sbrk(intptr_t increment){
	unsigned char* old_where;
	if(sbrk_where == NULL){
		sbrk_where = malloc(HEAP_SIZE);
		if(sbrk_where == NULL){
			return (void*)-1;
		}
	}
	if(increment == 0) return sbrk_where + sbrk_increment;
	old_where = sbrk_where + sbrk_increment;
	sbrk_increment += increment;
	return old_where;
}
