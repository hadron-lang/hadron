#include "errors.h"

CLIError *clierror(int argc, ...) {
	size_t s;
	va_list v0;
	va_list v1;
	va_start(v0, argc);
	for (small i = 1; i <= argc; i++) {
		s += strlen(va_arg(v0, string));
	}; va_end(v0);
	string e = malloc(s+1);
	strcpy(e, "");
	va_start(v1, argc);
	for (small i = 1; i <= argc; i++) {
		strcat(e, va_arg(v1, string));
	}; va_end(v1);
	CLIError *err = malloc(sizeof(CLIError));
	err->type = CLIERROR;
	err->data = e;
	return err;
}

Error *error(string name, string file, int line, int argcount, ...) {
	string n = malloc(strlen("Error") + strlen(name) + 1);
	strcpy(n, name);
	strcat(n, "Error");
	va_list v0;
	va_list v1;
	va_start(v0, argcount);
	size_t s = strlen(name);
	for (small i = 1; i <= argcount; i++) {
		s += strlen(va_arg(v0, string)) + 4;
	}; va_end(v0);
	string e = malloc(s + 1);
	strcpy(e, "");
	va_start(v1, argcount);
	for (small i = 1; i <= argcount; i++) {
		strcat(e, va_arg(v1, string));
	}; va_end(v1);
	Error *err = malloc(sizeof(Error));
	err->type = ERROR;
	err->name = n;
	err->file = file;
	err->data = e;
	err->line = line;
	return err;
};
