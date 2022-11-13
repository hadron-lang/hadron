#ifndef __LANG_PARSER_H
#define __LANG_PARSER_H 1

#include "./types.h"
#include "./errors.h"

Result *parse(string, TArray *);
Typed *walk(void);
void initParser(string, TArray *);
// Token next(void);
// Token peekNext(void);
// Token peekPrev(void);
// Token current(void);
// bool match_token(Type);
// bool end(void);
// bool start(void);

#endif
