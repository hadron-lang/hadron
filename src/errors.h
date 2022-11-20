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
	int line;
	string file;
	string name;
	string data;
} Error;
typedef struct CLIError {
	ErrorType type;
	string data;
} CLIError;
typedef struct ErrorLike {
	ErrorType type;
} ErrorLike;

extern CLIError *clierror(int arg_count, ...);
extern Error *error(string error_name, string file, int line, int arg_count, ...);

#endif
