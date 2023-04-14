#ifndef __HADRON_STACK
#define __HADRON_STACK 1

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern void stack_init(void);
extern void stack_push(char *name);
extern void stack_pop(void);
extern void stack_clear(void);
extern void stack_print(void);

#endif
