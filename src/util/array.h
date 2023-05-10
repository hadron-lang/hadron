#ifndef __LANG_ARRAY
#define __LANG_ARRAY 1

#include <malloc.h>

typedef struct Array {
	int	   l; // length
	int	   s; // size
	void **a; // array
} Array;

extern Array *newArray(int array_size);
extern Array *clearArray(Array *array);
extern void	  trimArray(Array *array);
extern void	  pushArray(Array *array, void *element);
extern void	  freeArray(Array *array);
extern void	  removeArray(Array *array, int index);
extern void	 *popArray(Array *array);
extern void	 *lastArray(Array *array);
extern void	 *getArray(Array *array, int index);

#endif
