#ifndef __HADRON_STACK
#define __HADRON_STACK 1

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// declare strdup for C99 compatibility
char *strdup(const char *s);

extern void stack_init(void);
extern void stack_push(char *name);
extern void stack_pop(void);
extern void stack_clear(void);
extern void stack_print(void);

#endif
