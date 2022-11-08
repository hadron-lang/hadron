#ifndef __LANG_TOKENIZER
#define __LANG_TOKENIZER 1

#include <string.h>

#include "types.h"

extern TArray* tokenize(string);
static char next(void);
static char peekNext(void);
static char peekPrev(void);
static char current(void);
static void initTokenizer(string);
static void addToken(Type);
static bool match(char);
static bool end(void);
static bool start(void);
static bool isAlpha(char);
static bool isDec(char);
static bool isBin(char);
static bool isOct(char);
static bool isHex(char);

#endif
