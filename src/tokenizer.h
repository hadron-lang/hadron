#ifndef __LANG_TOKENIZER
#define __LANG_TOKENIZER 1

#include <string.h>

#include "types.h"
#include "errors.h"

extern Result *tokenize(string);
void initTokenizer(string);
char next(void);
char peekNext(void);
char peekPrev(void);
char current(void);
void addToken(Type);
bool match(char);
bool end(void);
bool start(void);
bool isAlpha(char);
bool isDec(char);
bool isBin(char);
bool isOct(char);
bool isHex(char);

#endif
