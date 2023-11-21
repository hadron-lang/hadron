#ifndef __LANG_TYPES_H
#define __LANG_TYPES_H 1

#include "util/array.h"
#include <stdarg.h>
#include <string.h>

typedef unsigned char boolean;

#define true  (boolean)1
#define false (boolean)0

typedef struct Value {
	boolean error;
	void   *value;
} Value;

typedef enum __attribute__((__packed__)) Types {
	UNDEF,

	// COMPARE
	CMP_EQ,	 //* ==
	CMP_GT,	 //* >
	CMP_LT,	 //* <
	CMP_NEQ, //* !=
	CMP_LEQ, //* <=
	CMP_GEQ, //* >=

	// ASSIGN
	EQ,		   //* =
	ADD_EQ,	   //* +=
	SUB_EQ,	   //* -=
	MUL_EQ,	   //* *=
	DIV_EQ,	   //* /=
	INCR,	   //* ++
	DECR,	   //* --
	LAND_EQ,   //* &&=
	LOR_EQ,	   //* ||=
	BAND_EQ,   //* &=
	BOR_EQ,	   //* |=
	BXOR_EQ,   //* ^=
	REM_EQ,	   //* %=
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
	RETURN,

	// TYPE
	STR,	  //* "hello world"
	NAME,	  //* var_name
	DEC,	  //* 123
	DOT,	  //* .
	DELMT,	  //* ;
	NEWLINE,  //* \n todo remove
	SEP,	  //* ,
	AT,		  //* @
	HASH,	  //* #
	QUERY,	  //* ?
	LBRACKET, //* [
	RBRACKET, //* ]
	LPAREN,	  //* (
	RPAREN,	  //* )
	LCURLY,	  //* {
	RCURLY,	  //* }
	HEX,	  //* 0xff
	OCTAL,	  //* 0o77
	BIN,	  //* 0b1111
	COLON,	  //* :
	_EOF,	  //* \0

	// OPERATION
	ADD,	//* +
	SUB,	//* -
	MUL,	//* *
	DIV,	//* /
	LAND,	//* &&
	LOR,	//* ||
	BAND,	//* &
	BOR,	//* |
	BXOR,	//* ^
	BNOT,	//* ~ (unary)
	LNOT,	//* ! (unary)
	LSHIFT, //* <<
	RSHIFT, //* >>
	POW,	//* **
	REM		//* %
} Type;

typedef struct Position {
	int line;
	int start;
	int end;
	int absStart;
	int absEnd;
} Position;

typedef struct Token {
	Type	 type;
	Position pos;
} Token;

typedef enum __attribute__((__packed__)) AST_Types {
	PROGRAM,
	IMPORT_DECLARATION,
	IMPORT_SPECIFIER,
	ASSIGNMENT_EXPRESSION,
	CALL_EXPRESSION,
	BINARY_EXPRESSION,
	FUNCTION_DECLARATION,
	ASYNC_FUNCTION_DECLARATION,
	CLASS_DECLARATION,
	LITERAL,
	STRING_LITERAL,
	IDENTIFIER,
	TYPED_IDENTIFIER,
	EXPRESSION_STATEMENT,
	RETURN_STATEMENT
} AST_Type;

typedef struct Typed {
	AST_Type type;
} Typed;

typedef struct Identifier {
	AST_Type type;
	char	*name;
} Identifier;

typedef struct TypedIdentifier {
	AST_Type type;
	char	*name;
	char	*kind;
} TypedIdentifier;

typedef struct Literal {
	AST_Type type;
	char	*value;
} Literal;

typedef struct StringLiteral {
	AST_Type type;
	char	*value;
} StringLiteral;

typedef struct Program {
	AST_Type type;
	Array	*body;
} Program;

typedef struct ImportSpecifier {
	AST_Type	type;
	Identifier *name;
	Identifier *local;
} ImportSpecifier;

typedef struct ImportDeclaration {
	AST_Type	   type;
	StringLiteral *source;
	Array		  *specifiers;
} ImportDeclaration;

typedef struct FunctionDeclaration {
	AST_Type	type;
	boolean		async;
	Identifier *name;
	Array	   *params;
	Array	   *body;
} FunctionDeclaration;

typedef struct ClassDeclaration {
	AST_Type	type;
	Identifier *name;
	Array	   *body;
} ClassDeclaration;

typedef struct CallExpression {
	AST_Type	type;
	Identifier *callee;
	Array	   *params;
} CallExpression;

typedef struct ExpressionStatement {
	AST_Type type;
	Typed	*expr;
} ExpressionStatement;

typedef struct ReturnStatement {
	AST_Type type;
	Typed *expr;
} ReturnStatement;

typedef enum AssignmentOperators {
	ASGN_EQ = 1,
	ASGN_ADD_EQ,
	ASGN_SUB_EQ,
	ASGN_MUL_EQ,
	ASGN_DIV_EQ,
	ASGN_LAND_EQ,
	ASGN_LOR_EQ,
	ASGN_BAND_EQ,
	ASGN_BOR_EQ,
	ASGN_BXOR_EQ,
	ASGN_REM_EQ,
	ASGN_RSHIFT_EQ,
	ASGN_LSHIFT_EQ
} AssignmentOperator;

typedef struct AssignmentExpression {
	AST_Type				 type;
	AssignmentOperator oper;
	Typed					*left;
	Typed					*right;
} AssignmentExpression;

typedef enum BinaryOperators {
	BIN_UNDEF,
	BIN_ADD,
	BIN_SUB,
	BIN_MUL,
	BIN_DIV,
	BIN_LAND,
	BIN_LOR,
	BIN_BAND,
	BIN_BOR,
	BIN_BXOR,
	BIN_REM,
	BIN_RSHIFT,
	BIN_LSHIFT,
	BIN_POW
} BinaryOperator;

typedef struct BinaryExpression {
	AST_Type	   type;
	BinaryOperator oper;
	Typed		  *left;
	Typed		  *right;
} BinaryExpression;

typedef struct Result {
	Array *errors;
	void  *data;
} Result;

extern char				   *getTokenContent(char *code, Token *);
extern void					freeProgram(Program *);
extern Program			   *initProgram(int);
extern FunctionDeclaration *initFunctionDeclaration(
	boolean async, Identifier *name, Array *params, Array *body);
extern ClassDeclaration *initClassDeclaration(Identifier *name, Array *body);
extern ImportSpecifier	*initImportSpecifier(
	 Identifier *name, Identifier *local_name);
extern ImportDeclaration *initImportDeclaration(
	StringLiteral *, Array *import_specifier_array);
extern AssignmentExpression *initAssignmentExpression(
	Typed *left, Typed *right, AssignmentOperator oper);
extern Literal			*initLiteral(char *value);
extern StringLiteral	*initStringLiteral(char *value);
extern Identifier		*initIdentifier(char *name);
extern TypedIdentifier	*initTypedIdentifier(char *name, char *kind);
extern CallExpression	*initCallExpression(Identifier *callee, Array *params);
extern BinaryExpression *initBinaryExpression(
	BinaryOperator oper, Typed *left, Typed *right);
extern ExpressionStatement *initExpressionStatement(Typed *expr);
extern ReturnStatement     *initReturnStatement(Typed *expr);

#endif
