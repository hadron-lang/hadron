#ifndef __LANG_PRINTTOKENS
#define __LANG_PRINTTOKENS 1

#include "../types.h"
#include <string.h>

void printTokens(string, TArray *);
void printAST(Program *, small depth);
void util_typelog(Typed *, small);
void util_log(Typed *, small, small);
void printErrors(Array *);

#endif
