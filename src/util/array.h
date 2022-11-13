#ifndef __LANG_ARRAY
#define __LANG_ARRAY 1

#include <malloc.h>

typedef struct Array {
	int length;
	int size;
	void **array;
} Array;

Array *newArray(int array_size);
void initArray(void *array_like, int array_size);
void trimArray(void *array_like);
void pushArray(void *array_like, void *element);
void freeArray(void *array_like);

#endif
