#include "parser.h"

Parser parser;

void initParser(TArray *tokens) {
	parser.tokens = tokens;
	Program *program = initProgram(tokens->length/4);
	parser.program = program;
};

Typed *walk(void) {
	return NULL;
}

Program *parse(TArray *tokens) {
	initParser(tokens);

	ImportSpecifier *spec = initImportSpecifier("get", "get");
	ImportSpecifierArray *speca = malloc(sizeof(ImportSpecifierArray));
	initArray(speca, 1);
	push(speca, spec);
	ImportDeclaration *decl = initImportDeclaration("http", speca);
	push(&parser.program->body, decl);

	return parser.program;
};
