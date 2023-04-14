#ifndef __LANG_STR
#define __LANG_STR 1

#include "../types.h"

extern size_t utflen(string str);
extern string utfsubstr(string src, int start, int length);
extern string utfcat(string a, string b);
extern string substr(string src, int start, int length);

#endif
