#ifndef __LANG_PYTHON
#define __LANG_PYTHON 1

#include "../types.h"

// define public structs and enums here
typedef struct Python {
	size_t x;
	size_t y;
} Python;

typedef enum __attribute__((__packed__))RandomTypes {
	PY_DEFAULT, // leave the 0th element for errors / default stuff
	PY_A,
	PY_B,
	PY_C
} RandomType;

// declare public functions here
extern Result* py_compile(Program *); // probably should be the only public function

#endif
