#include "print.h"

static void util_typelog(Typed *, small);
static void util_log(Typed *, small, small);

void printTokens(string code, Array *tokens) {
	for (int i = 0; i < tokens->l; i++) {
		int len = ((Token *)tokens->a[i])->end - ((Token *)tokens->a[i])->start;
		string substr = malloc(len+1);
		memcpy(substr, &code[((Token *)tokens->a[i])->start], len);
		substr[len] = '\0';
		printf("type: %i line: %i \x1b[7m%s\x1b[0m\n", ((Token *)tokens->a[i])->type, ((Token*)tokens->a[i])->line, substr);
		free(substr);
	}
}

void util_typelog(Typed *v, small indent) {
	for (small i = 0; i < indent; i++) printf("  ");
	switch (v->type) {
		case PROGRAM:
			printf("\x1b[95m[Program]\x1b[0m { \x1b[94m...\x1b[0m }\n");
			break;
		case IMPORT_DECLARATION:
			printf("\x1b[95m[ImportDeclaration]\x1b[0m { \x1b[94m...\x1b[0m ");
			break;
		case IMPORT_SPECIFIER:
			printf("\x1b[95m[ImportSpecifier]\x1b[0m { \x1b[94m...\x1b[0m ");
			break;
		default: break;
	}
}

void util_log(Typed *v, small indent, small depth) {
	if (indent == depth) return util_typelog(v, indent);
	for (small i = 0; i < indent; i++) printf("  ");
	switch (v->type) {
		case PROGRAM: {
			printf("\x1b[95m[Program]\x1b[0m {\n");
			for (small i = 0; i < indent; i++) printf("  ");
			for (int i = 0; i < ((Program *)v)->body->l; i++) {
				util_log(((Program *)v)->body->a[i], indent+1, depth);
				printf("}\n");
			}
			printf("}\n");
			break;
		} case IMPORT_DECLARATION: {
			printf("\x1b[95m[ImportDeclaration]\x1b[0m {\n");
			ImportDeclaration *decl = (ImportDeclaration *)v;
			for (small i = 0; i < indent+1; i++) printf("  ");
			printf("\x1b[96msource\x1b[0m: \x1b[92m\"%s\"\x1b[0m\n", decl->source);
			for (int i = 0; i < decl->specifiers->l; i++) {
				util_log(decl->specifiers->a[i], indent+1, depth);
				printf("}\n");
			}
			for (small i = 0; i < indent; i++) printf("  ");
			break;
		} case IMPORT_SPECIFIER: {
			printf("\x1b[95m[ImportSpecifier]\x1b[0m {\n");
			ImportSpecifier *spec = (ImportSpecifier *)v;
			for (small i = 0; i < indent+1; i++) printf("  ");
			printf("\x1b[96mname\x1b[0m: \x1b[92m\"%s\"\x1b[0m\n", spec->name);
			for (small i = 0; i < indent+1; i++) printf("  ");
			printf("\x1b[96mlocal\x1b[0m: \x1b[92m\"%s\"\x1b[0m\n", spec->local);
			for (small i = 0; i < indent; i++) printf("  ");
		}
	}
}

void printAST(Program *p, small depth) {
	util_log((Typed *)p, 0, depth);
	printf("\n");
}

static void repeat(string c, int count) {
	for (int i = 0; i < count; i++) printf("%s", c);
}

static void codeError(Error *e) {
	string white = "\x1b[38;2;157;165;179m";
	string _white = "\x1b[48;2;157;165;179m";
	string red = "\x1b[38;2;255;0;0m";
	string black = "\x1b[38;2;0;0;0m";
	string blue = "\x1b[38;2;0;0;255m";
	string clear = "\x1b[0m";
	string bold = "\x1b[1m";
	FILE *fp = fopen(e->file, "r");
	char *line;
	size_t len;
	size_t read;
	int l = 0;
	if (fp == NULL) {
		printf("weird\n");
		return;
	}
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	size_t errlen = strlen(e->file) + strlen(e->name) + strlen(e->data) + 14;
	if (errlen < w.ws_col) {
		printf("%s────┬─%s%s%s %s %s%s─%s%s%s %s %s%s─%s%s%s %s %s%s",
			white, _white, bold, blue, e->file, clear,
			white, _white, bold, red, e->name, clear,
			white, _white, bold, black, e->data, clear, white
		);
		repeat("─", w.ws_col - errlen);
		printf("%s", clear);
	}
	printf("\n");
	while ((read = getline(&line, &len, fp)) != -1UL) {
		++l;
		if (l == e->line-1 || l == e->line+1) {
			printf("%s %i │%s %s", white, l, clear, line);
		} else if (l == e->line) {
			printf("%s %i │%s %s", white, l, clear, line);
		}
	}
	if (w.ws_col >= 5) {
		printf("%s────┴", white);
		repeat("─", w.ws_col - 5);
		printf("%s\n", clear);
	}
}

void printErrors(Array *array) {
	for (int i = 0; i < array->l; i++) {
		switch (((ErrorLike *)array->a[i])->type) {
			case CLIERROR: {
				CLIError *error = (CLIError *)array->a[i];
				printf("\x1b[1;7;91m CLIError \x1b[0;91m %s\x1b[0m\n", error->data);
				break;
			} case ERROR: {
				Error *error = (Error *)array->a[i];
				codeError(error);
				break;
			}
		}
		// if (error->line <= 0) {
		// 	printf("%s\n", error->data);
		// } else {
		// 	codeError(error);
		// }
	}
}
