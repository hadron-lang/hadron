#include "array.h"

Array *newArray(int s) {
	Array *p = malloc(sizeof(Array));
	initArray(p, s);
	return p;
}

void initArray(Array *a, int s) {
	a->a = malloc(sizeof(void *) * s);
	a->l = 0;
	a->s = s;
}

void trimArray(Array *a) {
	a->a = realloc(a->a, sizeof(void *) * a->l);
	a->s = a->l;
}

void pushArray(Array *a, void *el) {
	if (a->s == a->l) {
		a->s *= 1.5;
		a->a = realloc(a->a, sizeof(void *) * a->s);
	}
	a->a[a->l++] = el;
}

// Warning: `a` will be unusable after this point
void freeArray(Array *a) {
	for (int i = 0; i < a->l; i++) free(a->a[i]);
	free(a->a); free(a);
}

void *popArray(Array *a) {
	return a->l > 0 ? a->a[--a->l] : NULL;
}
