#ifndef __LANG_PARSER_H
#define __LANG_PARSER_H 1

#include "./types.h"

Program *parse(TArray *);
Typed *walk(void);
void initParser(TArray *);

#endif
