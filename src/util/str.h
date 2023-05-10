#ifndef __LANG_STR
#define __LANG_STR 1

#include "../types.h"

extern size_t utflen(char *str);
extern char	 *utfsubstr(char *src, int start, int length);
extern char	 *utfcat(char *a, char *b);
extern char	 *substr(char *src, int start, int length);

#endif
