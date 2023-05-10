#include "src/errors.h"
#include "src/parser.h"
#include "src/tokenizer.h"
#include "src/types.h"
#include "src/util/print.h"
#include "src/util/str.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef enum __attribute__((__packed__)) Langs {
	L_UNKN,
	L_JS,
	L_C,
	L_ASM
} Lang;

typedef enum __attribute__((__packed__)) Modes {
	COMPILE,
	INTERPRET,
	DEBUG
} Mode;

// declare strcasecmp for C99 compatibility
int strcasecmp(const char *s1, const char *s2);

extern int main(int, char *[]);
