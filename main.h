#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "src/tokenizer.h"
#include "src/types.h"
#include "src/util/printtokens.h"

typedef enum Langs {
	L_JS = 1,
	L_C,
	L_ASM,
	L_UNKN
} Lang;

typedef struct Args {
	Lang lang;
	string file;
	small mode;
} Args;

int main(int, string *);
int CLIError(int, ...);
bool isLang(string);
bool isFile(string);
Lang parseLang(string);
