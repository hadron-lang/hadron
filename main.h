#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "src/tokenizer.h"
#include "src/types.h"
#include "src/util/printtokens.h"

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

extern int main(small, string *);
static int CLIError(int, ...);
static bool isLang(string);
static bool isFile(string);
static Lang parseLang(string);
