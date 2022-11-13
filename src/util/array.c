#include "array.h"

Array *newArray(int s) {
	Array *p = malloc(sizeof(Array));
	initArray(p, s);
	return p;
}

void initArray(void *a, int s) {
	Array *p = (Array *)a;
	p->array = malloc(sizeof(void *) * s);
	p->length = 0;
	p->size = s;
}

void trimArray(void *a) {
	Array *p = (Array *)a;
	p->array = realloc(p->array, sizeof(void *) * p->length);
	p->size = p->length;
}

void pushArray(void *a, void *el) {
	Array *p = (Array *)a;
	if (p->size == p->length) {
		p->size *= 1.5;
		p->array = realloc(p->array, sizeof(void *) * p->size);
	}
	p->array[p->length++] = el;
}

void freeArray(void *a) {
	Array *p = (Array *)a;
	for (int i = 0; i < p->length; i++) free(p->array[i]);
	free(p->array);
}
