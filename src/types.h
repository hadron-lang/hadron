#ifndef __LANG_TYPES_H
#define __LANG_TYPES_H 1

#include <stdarg.h>
#include <string.h>
#include "util/array.h"

typedef unsigned char small;
typedef unsigned char bool;
typedef char *string;

#define true (bool)1
#define false (bool)0

typedef struct Value {
	bool error;
	void *value;
} Value;

typedef enum __attribute__((__packed__)) Types {
	UNDEF,

	// COMPARE
	CMP_EQ,  //* ==
	CMP_GT,  //* >
	CMP_LT,  //* <
	CMP_NEQ, //* !=
	CMP_LEQ, //* <=
	CMP_GEQ, //* >=

	// ASSIGN
	EQ,        //* =
	ADD_EQ,    //* +=
	SUB_EQ,    //* -=
	MUL_EQ,    //* *=
	DIV_EQ,    //* /=
	INCR,      //* ++
	DECR,      //* --
	LAND_EQ,   //* &&=
	LOR_EQ,    //* ||=
	BAND_EQ,   //* &=
	BOR_EQ,    //* |=
	BXOR_EQ,   //* ^=
	REM_EQ,    //* %=
	RSHIFT_EQ, //* >>=
	LSHIFT_EQ, //* <<=

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
	AS,
	ASYNC,
	RET,

	// TYPE
	STR,      //* "hello world"
	NAME,     //* var_name
	DEC,      //* 123
	DOT,      //* .
	DELMT,    //* ;
	NEWLINE,  //* \n
	SEP,      //* ,
	AT,       //* @
	HASH,     //* #
	QUERY,    //* ?
	BRACKET,  //* [
	$BRACKET, //* ]
	PAREN,    //* (
	$PAREN,   //* )
	CURLY,    //* {
	$CURLY   ,//* }
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

typedef struct Position {
	int line;
	int start;
	int end;
	int absStart;
	int absEnd;
} Position;

typedef struct Token {
	Type type;
	Position pos;
} Token;

typedef enum __attribute__((__packed__)) AST_Types {
	PROGRAM,
	IMPORT_DECLARATION,
	IMPORT_SPECIFIER,
	ASSIGNMENT_EXPRESSION,
	FUNCTION_DECLARATION,
	LITERAL,
	STRING_LITERAL,
	IDENTIFIER
} AST_Type;

typedef struct Typed {
	AST_Type type;
} Typed;

typedef struct Identifier { // extends Typed
	AST_Type type;
	string name;
	string kind;
} Identifier;

typedef struct Literal { // extends Typed
	AST_Type type;
	string value;
} Literal;

typedef struct StringLiteral { // extends Typed
	AST_Type type;
	string value;
} StringLiteral;

typedef struct Program { // extends Typed
	AST_Type type;
	Array *body;
} Program;

typedef struct ImportSpecifier { // extends Typed
	AST_Type type;
	Identifier *name;
	Identifier *local;
} ImportSpecifier;

typedef struct ImportDeclaration { // extends Typed
	AST_Type type;
	StringLiteral *source;
	Array *specifiers;
} ImportDeclaration;

typedef struct FunctionDeclaration {
	AST_Type type;
	bool async;
	Identifier *name;
	Array *params;
	Array *body;
} FunctionDeclaration;

typedef struct AssignmentExpression { // extends Typed
	AST_Type type;
	Typed *left;
	Typed *right;
} AssignmentExpression;

typedef struct Result {
	Array *errors;
	void *data;
} Result;

extern string getTokenContent(string code, Token*);
extern void freeProgram(Program *);
extern Program *initProgram(int);
extern FunctionDeclaration *initFunctionDeclaration(bool async, Identifier *name, Array *params, Array *body);
extern ImportSpecifier *initImportSpecifier(Identifier *name, Identifier *local_name);
extern ImportDeclaration *initImportDeclaration(StringLiteral *, Array *import_specifier_array);
extern AssignmentExpression *initAssignmentExpression(Typed *left, Typed *right);
extern Literal *initLiteral(string value);
extern StringLiteral *initStringLiteral(string value);
extern Identifier *initIdentifier(string name, string kind);

#endif
