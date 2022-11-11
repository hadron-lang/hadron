#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "src/tokenizer.h"
#include "src/parser.h"
#include "src/types.h"
#include "src/errors.h"
#include "src/util/print.h"

typedef enum __attribute__((__packed__)) Langs {
	L_UNKN,
	L_JS,
	L_C,
	L_ASM
} Lang;

typedef struct Args {
	Lang lang;
	string file;
	small mode;
} Args;

int main(int, string *);
int check(Array *);
bool isLang(string);
bool isFile(string);
Lang parseLang(string);
