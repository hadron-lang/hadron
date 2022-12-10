#include "str.h"

size_t utflen(string s) {
   size_t l = 0; for (size_t i = 0; i < strlen(s); i++, l++) {
		if      ((s[i] & 0xe0) == 0xc0) l -= 1;
		else if ((s[i] & 0xf0) == 0xe0) l -= 2;
		else if ((s[i] & 0xf8) == 0xf0) l -= 3;
   }; return l;
}
string utfsubstr(string s, int start, int l) {
	size_t rl = 0;
	for (int i = 0; i < l; i++, rl++) {
		char c = s[start+rl];
		if      ((c & 0xe0) == 0xc0) rl+=1;
		else if ((c & 0xf0) == 0xe0) rl+=2;
		else if ((c & 0xf8) == 0xf0) rl+=3;
	}
	string sub = malloc(rl);
	memcpy(sub, s+start, rl);
	sub[rl] = 0;
	return sub;
}
string utfcat(string a, string b) {
	size_t len = strlen(a) + strlen(b);
	string cat = malloc(len);
	strcpy(cat, a);
	strcat(cat, b);
	cat[len] = 0;
	return cat;
}
string substr(string s, int start, int l) {
	string sub = (string)malloc(l+1);
	memcpy(sub, s+start, l);
	sub[l] = 0;
	return sub;
}
