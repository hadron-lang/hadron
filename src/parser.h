#ifndef __LANG_PARSER_H
#define __LANG_PARSER_H 1

#include "./types.h"
#include "./errors.h"
#include "./util/str.h"

typedef struct Res {
	__attribute__((__packed__))enum ResTypes {
		RERROR,
		RTOKEN
	} type;
	void *value;
} Res;

extern Result *parse(string, Array *, string);

#endif
