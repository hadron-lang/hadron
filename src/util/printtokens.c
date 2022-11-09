#include "printtokens.h"

void printTokens(string code, TArray *tokens) {
	for (int i = 0; i < tokens->length; i++) {
		int len = tokens->array[i]->end - tokens->array[i]->start;
		string substr = malloc(len+1);
		memcpy(substr, &code[tokens->array[i]->start], len);
		substr[len] = '\0';
		printf("type: %i \x1b[7m%s\x1b[0m\n", tokens->array[i]->type, substr);
		free(substr);
	}
}

void util_log(Typed *v, small indent) {
	for (small i = 0; i < indent; i++) printf("  ");
	switch (v->type) {
		case PROGRAM: {
			printf("\x1b[95m[Program]\x1b[0m {\n");
			for (small i = 0; i < indent; i++) printf("  ");
			for (int i = 0; i < ((Program *)v)->length; i++) {
				util_log(((Program *)v)->array[i], indent+1);
				printf("},\n");
			}
			printf("}\n");
			break;
		} case IMPORT_DECLARATION: {
			printf("\x1b[95m[ImportDeclaration]\x1b[0m {\n");
			ImportDeclaration *decl = (ImportDeclaration *)v;
			for (small i = 0; i < indent+1; i++) printf("  ");
			printf("\x1b[96msource\x1b[0m: \x1b[92m\"%s\"\x1b[0m,\n", decl->source);
			for (int i = 0; i < decl->specifiers->length; i++) {
				util_log((Typed *)(decl->specifiers->array[i]), indent+1);
				printf("}%s\n", i < decl->specifiers->length ? "," : "");
			}
			for (small i = 0; i < indent; i++) printf("  ");
			break;
		} case IMPORT_SPECIFIER: {
			printf("\x1b[95m[ImportSpecifier]\x1b[0m {\n");
			ImportSpecifier *spec = (ImportSpecifier *)v;
			for (small i = 0; i < indent+1; i++) printf("  ");
			printf("\x1b[96mname\x1b[0m: \x1b[92m\"%s\"\x1b[0m,\n", spec->name);
			for (small i = 0; i < indent+1; i++) printf("  ");
			printf("\x1b[96mlocalName\x1b[0m: \x1b[92m\"%s\"\x1b[0m\n", spec->localName);
			for (small i = 0; i < indent; i++) printf("  ");
		}
	}
}

void printAST(Program *p) {
	util_log((Typed *)p, 0);
	printf("\n");
}
