#ifndef __LANG_TYPES_H
#define __LANG_TYPES_H 1

#include <malloc.h>

typedef unsigned char small;
typedef unsigned char bool;
typedef char *string;

#define true (bool)1
#define false (bool)0

typedef enum __attribute__((__packed__)) Types {
	// COMPARE
	CMP_EQ = 10,//* ==
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
	NAME,     //* var_name
	DEC,      //* 123
	DOT,      //* .
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
	int size;
	Token **array;
} TArray;

typedef struct Tokenizer {
	int line;
	int iterator;
	int tokenStart;
	int len;
	TArray *tokens;
	string code;
} Tokenizer;

typedef struct Parser {

} Parser;

typedef enum AST_Types {
	PROGRAM = 1,
	IMPORT_DECLARATION,
	IMPORT_SPECIFIER
} AST_Type;

typedef struct Typed {
	AST_Type type;
} Typed;

typedef struct Program {
	AST_Type type;
	int length;
	void **array;
} Program;

typedef struct ImportSpecifier {
	AST_Type type; // type always coming first
	string name;
	string localName;
} ImportSpecifier;

typedef struct ImportSpecifierArray {
	ImportSpecifier **array;
	int length;
} ImportSpecifierArray;

typedef struct ImportDeclaration {
	AST_Type type;
	string source;
	ImportSpecifierArray *specifiers;
} ImportDeclaration;

TArray *initTArray(int);
void pushToken(TArray *, Token *);
void freeTArray(TArray *);

#endif
