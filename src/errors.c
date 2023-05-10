#include "errors.h"

CLIError *clierror(int argc, ...) {
	size_t	s;
	va_list v0;
	va_list v1;
	va_start(v0, argc);
	for (int i = 1; i <= argc; i++) {
		s += strlen(va_arg(v0, char *));
	};
	va_end(v0);
	char *e = malloc(s + 1);
	strcpy(e, "");
	va_start(v1, argc);
	for (int i = 1; i <= argc; i++) {
		strcat(e, va_arg(v1, char *));
	};
	va_end(v1);
	CLIError *err = malloc(sizeof(CLIError));
	err->type	  = CLIERROR;
	err->data	  = e;
	return err;
}

Error *error(char *name, char *file, char *data, Token *token) {
	Error *err = malloc(sizeof(Error));
	char  *n   = malloc(strlen("Error") + strlen(name) + 1);
	strcpy(n, name);
	strcat(n, "Error");
	err->type  = ERROR;
	err->name  = n;
	err->file  = file;
	err->data  = data;
	err->token = token;
	return err;
}
