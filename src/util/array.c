#include "array.h"

static void initArray(Array *, int);

Array *newArray(int s) {
	Array *p = malloc(sizeof(Array));
	initArray(p, s);
	return p;
}

void initArray(Array *a, int s) {
	int size = s < 2 ? 2 : s;
	a->a = malloc(sizeof(void *) * size);
	a->l = 0;
	a->s = size;
}

void trimArray(Array *a) {
	a->a = realloc(a->a, sizeof(void *) * a->l);
	a->s = a->l;
}

void pushArray(Array *a, void *el) {
	if (a->s == a->l) {
		a->s *= 1.5;
		a->a = realloc(a->a, sizeof(void *) * a->s);
	}; a->a[a->l++] = el;
}

void removeArray(Array *a, int i) {
	if (i < 0 && i >= a->l) return;
	a->a[i] = NULL;
}

Array *clearArray(Array *a) {
	Array *r = newArray(a->s);
	for (int i = 0; i < a->l; i++) {
		void *x = a->a[i];
		if (x != NULL) pushArray(r, x);
	}; return r;
}

// @warning `array` will be unusable after this point
void freeArray(Array *a) {
	for (int i = 0; i < a->l; i++) free(a->a[i]);
	free(a->a); free(a);
}

void *popArray(Array *a) {
	return a->l > 0 ? a->a[--a->l] : NULL;
}

void *lastArray(Array *a) {
	return a->l > 0 ? a->a[a->l-1] : NULL;
}

void *getArray(Array *a, int i) {
	return i >= 0 && i < a->l ? a->a[i] : NULL;
}
