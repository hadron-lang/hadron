#ifndef __LANG_PRINTTOKENS
#define __LANG_PRINTTOKENS 1

#include <string.h>
#include "../types.h"
#include "array.h"

extern void printTokens(string file_contents, TArray *tokens);
extern void printAST(Program *program, small depth);
extern void printErrors(Array *);
void util_typelog(Typed *, small);
void util_log(Typed *, small, small);

#endif
