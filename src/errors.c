#include "errors.h"

string Error(string name, int argcount, ...) {
	string e0 = "\x1b[7;1;91m ";
	string e1 = "Error \x1b[0;91m ";
	string e2 = "\x1b[0m\n";
	va_list v0;
	va_list v1;
	va_start(v0, argcount);
	size_t s = strlen(e0) + strlen(name) + strlen(e1) + strlen(e2);
	for (small i = 1; i <= argcount; i++) {
		string tmp0 = "\x1b[0;91m";
		string tmp1 = va_arg(v0, string);
		s += strlen(tmp0) + strlen(tmp1);
	}
	va_end(v0);
	string e = malloc(s + 1);
	strcpy(e, e0);
	strcat(e, name);
	strcat(e, e1);
	va_start(v1, argcount);
	for (small i = 1; i <= argcount; i++) {
		string tmp = "\x1b[0;91m";
		strcat(e, tmp);
		strcat(e, va_arg(v1, string));
	}
	va_end(v1);
	strcat(e, e2);
	return e;
};
