#include "stack.h"
#ifndef __LANG_PRINTTOKENS
#define __LANG_PRINTTOKENS 1

#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../types.h"
#include "array.h"
#include "../errors.h"
#include "str.h"

// declare getline for C99 compatibility
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

extern void printTokens(char *file_contents, Array *tokens);
extern void printToken(Token *t);
extern void printAST(Program *program, int depth);
extern void printErrors(Array *);

extern void debug_log(char *function_name, char *code, Token *token);

#endif
