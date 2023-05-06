#ifndef __LANG_ERRORS
#define __LANG_ERRORS 1

#include <stdarg.h>
#include <string.h>
#include "types.h"

typedef enum __attribute__((__packed__)) ErrorTypes {
	ERROR,
	CLIERROR
} ErrorType;

typedef struct Error {
	ErrorType type;
	char *file;
	char *name;
	char *data;
	Token *token;
} Error;
typedef struct CLIError {
	ErrorType type;
	char *data;
} CLIError;
typedef struct ErrorLike {
	ErrorType type;
} ErrorLike;

extern CLIError *clierror(int arg_count, ...);
extern Error *error(char *error_name, char *file, char *data, Token* token);

#endif
