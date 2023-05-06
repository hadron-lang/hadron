#include "str.h"

size_t utflen(char *s) {
   size_t l = 0; for (size_t i = 0; i < strlen(s); i++, l++) {
		if      ((s[i] & 0xe0) == 0xc0) l -= 1;
		else if ((s[i] & 0xf0) == 0xe0) l -= 2;
		else if ((s[i] & 0xf8) == 0xf0) l -= 3;
   }; return l;
}
char *utfsubstr(char *s, int start, int l) {
	size_t rl = 0;
	for (int i = 0; i < l; i++, rl++) {
		char c = s[start+rl];
		if      ((c & 0xe0) == 0xc0) rl+=1;
		else if ((c & 0xf0) == 0xe0) rl+=2;
		else if ((c & 0xf8) == 0xf0) rl+=3;
	}
	char *sub = malloc(rl);
	memcpy(sub, s+start, rl);
	sub[rl] = 0;
	return sub;
}
char *utfcat(char *a, char *b) {
	size_t len = strlen(a) + strlen(b);
	char *cat = malloc(len+1);
	strcpy(cat, a);
	strcat(cat, b);
	cat[len] = 0;
	return cat;
}
char *substr(char *s, int start, int l) {
	char *sub = (char *)malloc(l+1);
	memcpy(sub, s+start, l);
	sub[l] = 0;
	return sub;
}
