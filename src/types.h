#ifndef __LANG_TYPES_H
#define __LANG_TYPES_H 1

#include <malloc.h>

#include "util/small.h"
#include "util/bool.h"
#include "util/string.h"

typedef enum Types {
	// COMPARE

	CMP_EQ = 10, //* ==
	CMP_GT,     //* >
	CMP_LT,     //* <
	CMP_NEQ,    //* !=
	CMP_LEQ,    //* <=
	CMP_GEQ,    //* >=
	// ASSIGN

	EQ,        // * =
	ADD_EQ,    // * +=
	SUB_EQ,    // * -=
	MUL_EQ,    // * *=
	DIV_EQ,    // * /=
	INCR,      // * ++
	DECR,      // * --
	LAND_EQ,   // * &&=
	LOR_EQ,    // * ||=
	BAND_EQ,   // * &=
	BOR_EQ,    // * |=
	BXOR_EQ,   // * ^=
	BNOT_EQ,   // * ~=
	REM_EQ,    // * %=
	RSHIFT_EQ, // * >>=
	LSHIFT_EQ, // * <<=
	// KEYWORD

	FOR,
	CLASS,
	FUNC,
	TRUE,
	FALSE,
	_NULL,
	WHILE,
	IF,
	DO,
	ELSE,
	FROM,
	IMPORT,
	NEW,
	AWAIT,
	// TYPE

	STR,      //* "hello world"
	NAME,
	BLOCK,    //* { ... }
	DEC,      //* 123
	CALL,     //* func (...)
	CHAIN,    //* .
	EXP,      //* (...)
	DELMT,    //* ;
	SEP,      //* ,
	AT,       //* @
	UNDEF,
	BRACKET,  //* []
	PAREN,    //* ()
	CBRACKET, //* {}
	HEX,      //* 0xff
	OCTAL,    //* 0o77
	BIN,      //* 0b1111
	COLON,    //* :
	_EOF,     //* \0
	// OPERATION

	ADD,    //* +
	SUB,    //* -
	MUL,    //* *
	DIV,    //* /
	LAND,   //* &&
	LOR,    //* ||
	BAND,   //* &
	BOR,    //* |
	BXOR,   //* ^
	BNOT,   //* ~
	LNOT,   //* !
	LSHIFT, //* <<
	RSHIFT, //* >>
	POW,    //* **
	REM     //* %
} Type;

typedef struct Token {
	Type type;
	int line;
	int start;
	int end;
} Token;

typedef struct TArray {
	int length;
	Token *array;
} TArray;

typedef struct Tokenizer {
	int line;
	int iterator;
	int tokenStart;
	int len;
	TArray tokens;
	string code;
} Tokenizer;

void initTArray(TArray *ptr) {
	ptr->array = malloc(0);
	ptr->length = 0;
}

void pushToken(TArray *ptr, Token token) {
	ptr->array = realloc(ptr->array, sizeof(Token) * (ptr->length+1));
	ptr->array[ptr->length] = token;
	ptr->length++;
}

void freeTArray(TArray *ptr) {
	free(ptr->array);
}

#endif
