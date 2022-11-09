#ifndef __LANG_PRINTTOKENS
#define __LANG_PRINTTOKENS 1

#include "../types.h"
#include <string.h>

void printTokens(string, TArray *);
void printAST(Program *);
void util_log(Typed *v, small indent);

#endif
