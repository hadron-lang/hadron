#include "stack.h"

static struct Stack {
	int length;
	int size;
	char **arr;
} stack;

void stack_init() {
	stack.length = 0;
	stack.size = 8;
	stack.arr = malloc(sizeof(char *) * 8);
}

void stack_push(char *name) {
	char *v = strdup(name);
	if (stack.length == stack.size) {
		stack.size *= 2;
		malloc(sizeof(char *) * stack.size);
	}
	stack.arr[stack.length++] = v;
}

void stack_pop() {
	free(stack.arr[--stack.length]);
}

void stack_clear() {
	while (stack.length) stack_pop();
	free(stack.arr);
}

void stack_print() {
	printf("Stack:\n");
	for (int i = 0; i < stack.length; i++) {
		printf(" - %s\n", stack.arr[i]);
	}
}
