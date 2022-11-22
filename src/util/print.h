#ifndef __LANG_PRINTTOKENS
#define __LANG_PRINTTOKENS 1

#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../types.h"
#include "array.h"
#include "../errors.h"
#include "../tokenizer.h"

extern void printTokens(string file_contents, Array *tokens);
extern void printAST(Program *program, small depth);
extern void printErrors(Array *);

#endif
