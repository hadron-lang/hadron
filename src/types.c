#include "types.h"

Program *initProgram(int s) {
	Program *prg = malloc(sizeof(Program));
	prg->type = PROGRAM;
	prg->body = newArray(s);
	return prg;
}
void freeProgram(Program *program) {
	for (int i = 0; i < program->body->l; i++) {
		switch (((Typed *)program->body->a[i])->type) {
			case PROGRAM: { break; }
			case IMPORT_DECLARATION: { break; }
			case IMPORT_SPECIFIER: { break; }
		}
	}
}

string getType(Type type, ...) {
	va_list v;
	va_start(v, type);
	string s = va_arg(v, string);
	string r = malloc(strlen(s) + 16);
	strcpy(r, s);
	string s0;
	switch (type) {
		case STR: s0 = "String"; break;
		case NAME: s0 = "Name"; break;
		case DEC: case HEX: case OCTAL: case BIN: s0 = "Number"; break;
		case BRACKET: s0 = "Bracket"; break;
		case PAREN: s0 = "Parenthesis"; break;
		case CBRACKET: s0 = "CurlyBracket"; break;
		default: s0 = "Undefined"; break;
	}
	strcat(r, s0);
	return r;
}

ImportSpecifier *initImportSpecifier(string name, string local) {
	ImportSpecifier *spec = malloc(sizeof(ImportSpecifier));
	spec->type = IMPORT_SPECIFIER;
	spec->name = name;
	spec->local = local;
	return spec;
}
ImportDeclaration *initImportDeclaration(string src, Array *speca) {
	ImportDeclaration *decl = malloc(sizeof(ImportSpecifier));
	decl->type = IMPORT_DECLARATION;
	decl->source = src;
	decl->specifiers = speca;
	return decl;
}
