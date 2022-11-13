#ifndef __LANG_PARSER_H
#define __LANG_PARSER_H 1

#include "./types.h"
#include "./errors.h"

extern Result *parse(string, TArray *);
void initParser(string, TArray *);
bool temp_end(void);
bool temp_start(void);
Token *temp_next(void);
Token *temp_peekNext(void);
Token *temp_peekPrev(void);
Token *temp_current(void);
bool temp_match(Type);

#endif
