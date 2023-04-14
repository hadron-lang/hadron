#include "python.h"

// declare private functions here
static void util_func0(void);
static void util_func1(void);

// define them here
void util_func0() {
	// do smth
}
void util_func1() {
	// do smth else
}

// main function (public)
Result *py_compile(Program *prog) {
	Result *res = malloc(sizeof(Result));
	res->errors = newArray(2);

	printf("%i\n", prog->type);

	util_func0();
	util_func1();
	// code
	// res->data = NULL;

	return res;
}
