#include "types.h"

string getTokenContent(string src, Token *t) {
	bool str = t->type == STR;
	int l = t->pos.absEnd - t->pos.absStart - (str ? 2 : 0);
	string sub = (string)malloc(l+1);
	memcpy(sub, src+t->pos.absStart + (str ? 1 : 0), l);
	sub[l] = 0;
	return sub;
}

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
