#include "array.h"

Array *newArray(int s) {
	Array *p = malloc(sizeof(Array));
	initArray(p, s);
	return p;
}

void initArray(Array *a, int s) {
	Array *p = (Array *)a;
	p->a = malloc(sizeof(void *) * s);
	p->l = 0;
	p->s = s;
}

void trimArray(Array *a) {
	Array *p = (Array *)a;
	p->a = realloc(p->a, sizeof(void *) * p->l);
	p->s = p->l;
}

void pushArray(Array *a, void *el) {
	Array *p = (Array *)a;
	if (p->s == p->l) {
		p->s *= 1.5;
		p->a = realloc(p->a, sizeof(void *) * p->s);
	}
	p->a[p->l++] = el;
}

void freeArray(Array *a) {
	Array *p = (Array *)a;
	for (int i = 0; i < p->l; i++) free(p->a[i]);
	free(p->a);
}
