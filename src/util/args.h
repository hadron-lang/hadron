#ifndef __LANG_ARGS_H
#define __LANG_ARGS_H 1

#include "./small.h"
#include "./string.h"

typedef enum Langs {
	L_JS = 1,
	L_C,
	L_ASM
} Lang;

typedef struct Args {
	Lang lang;
	string file;
	small mode;
} Args;

#endif
