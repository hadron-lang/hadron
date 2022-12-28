#include "print.h"
#include <math.h>

static void util_typelog(Typed *);
static void util_log(Typed *, small, small);
static void repeat(string, int);
static void tab(small);

void repeat(string c, int count) {
	for (int i = 0; i < count; i++) printf("%s", c);
}
int intl(int x) {
	if (x == 0) return 1;
	else {
		// printf("%i %f\n", x, ceil(log10(x*10)-1));
		return (int)ceil(log10(x*10+1)-1);
	};
}

void printToken(Token* t) {
	string key0 = "\x1b[94m";
	string key1 = "\x1b[96m";
	string value = "\x1b[93m";
	string clear = "\x1b[0m";
	string italic = "\x1b[3m";
	printf("%s\x1b[37mToken%s ", italic, clear);
	printf("{ %stype%s: ", key0, clear);
	printf("%s%i%s, %spos%s: { %sline%s: ", value, t->type, clear, key0, clear, key1, clear);
	printf("%s%i%s, %sstart%s: ", value, t->pos.line, clear, key1, clear);
	printf("%s%i%s, %send%s: ", value, t->pos.start, clear, key1, clear);
	printf("%s%i%s, %sabsStart%s: ", value, t->pos.end, clear, key1, clear);
	printf("%s%i%s, %sabsEnd%s: ", value, t->pos.absStart, clear, key1, clear);
	printf("%s%i%s }\n", value, t->pos.absEnd, clear);
}

void printTokens(string code, Array *tokens) {
	int align[8] = { 0, 0, 0, 0, 0, 0, 0, 10 };
	for (int i = 0; i < tokens->l; i++) {
		Token *t = (Token *)tokens->a[i];
		int l0 = intl(t->type);
		int l1 = intl(t->pos.line);
		int l2 = intl(t->pos.start);
		int l3 = intl(t->pos.end);
		int l4 = intl(t->pos.absStart);
		int l5 = intl(t->pos.absEnd);
		int l6 = intl(i);
		if (l0 > align[0]) align[0] = l0;
		if (l1 > align[1]) align[1] = l1;
		if (l2 > align[2]) align[2] = l2;
		if (l3 > align[3]) align[3] = l3;
		if (l4 > align[4]) align[4] = l4;
		if (l5 > align[5]) align[5] = l5;
		if (l6 > align[6]) align[6] = l6;
	}
	for (int i = 0; i < tokens->l; i++) {
		Token *t = (Token*)tokens->a[i];
		int len = t->pos.absEnd - t->pos.absStart;
		string substr = malloc(len);
		memcpy(substr, code+t->pos.absStart, len);
		substr[len] = 0;
		int rlen = utflen(substr);
		string key0 = "\x1b[94m";
		string key1 = "\x1b[96m";
		string value = "\x1b[93m";
		string clear = "\x1b[0m";
		string token = "\x1b[92m";
		string italic = "\x1b[3m";
		if (rlen > align[7]) {
			free(substr);
			substr = utfsubstr(code, t->pos.absStart, align[7]-3);
			substr = utfcat(substr, "\x1b[90m...");
			rlen = align[7];
		}
		if (t->type == _EOF) { strcpy(substr, "\\0"); rlen = 2; };
		printf("%s\x1b[37mToken%s", italic, clear);
		// printf("%s\x1b[37mToken%i%s", italic, i, clear);
		// repeat(" ", align[6]-intl(i));
		printf("<%s%s%s>", token, substr, clear);
		repeat(" ", align[7] - rlen);
		printf("{ %stype%s: ", key0, clear);
		repeat(" ", align[0] - intl(t->type));
		printf("%s%i%s, %spos%s: { %sline%s: ", value, t->type, clear, key0, clear, key1, clear);
		repeat(" ", align[1] - intl(t->pos.line));
		printf("%s%i%s, %sstart%s: ", value, t->pos.line, clear, key1, clear);
		repeat(" ", align[2] - intl(t->pos.start));
		printf("%s%i%s, %send%s: ", value, t->pos.start, clear, key1, clear);
		repeat(" ", align[3] - intl(t->pos.end));
		printf("%s%i%s, %sabsStart%s: ", value, t->pos.end, clear, key1, clear);
		repeat(" ", align[4] - intl(t->pos.absStart));
		printf("%s%i%s, %sabsEnd%s: ", value, t->pos.absStart, clear, key1, clear);
		repeat(" ", align[5] - intl(t->pos.absEnd));
		printf("%s%i%s }\n", value, t->pos.absEnd, clear);
		free(substr);
	}
}

void util_typelog(Typed *v) {
	switch (v->type) {
		case PROGRAM:
			printf("\x1b[95m[Program]\x1b[0m { \x1b[94m...\x1b[0m }\n");
			break;
		case IMPORT_DECLARATION:
			printf("\x1b[95m[ImportDeclaration]\x1b[0m { \x1b[94m...\x1b[0m }\n");
			break;
		case IMPORT_SPECIFIER:
			printf("\x1b[95m[ImportSpecifier]\x1b[0m { \x1b[94m...\x1b[0m }\n");
			break;
		case ASSIGNMENT_EXPRESSION:
			printf("\x1b[95m[AssignmentExpression]\x1b[0m { \x1b[94m...\x1b[0m }\n");
			break;
		case LITERAL:
			printf("\x1b[93m%s\x1b[0m\n", ((Literal *)v)->value);
			break;
		default: break;
	}
}

void util_linelog(Typed *v) {
	switch (v->type) {
		case STRING_LITERAL: {
			printf("\x1b[92m\"%s\"\x1b[0m\n", ((StringLiteral *)v)->value); break;
		} case LITERAL: {
			printf("\x1b[93m%s\x1b[0m\n", ((Literal *)v)->value); break;
		} case IDENTIFIER: {
			Identifier *id = (Identifier *)v;
			id->kind
				? printf("\x1b[94;3m%s\x1b[0m <\x1b[34;1m%s\x1b[0m>\n", id->name, id->kind)
				: printf("\x1b[94;3m%s\x1b[0m\n", id->name);
			break;
		} default: printf("\x1b[93mnull\x1b[0m\n"); break;
	}
}

void tab(small indent) {
	for (small i = 0; i < indent; i++) {
		// printf("%s  ", i%2 ? "" : "");
		printf("\x1b[30m  \x1b[0m");
	}
}

void util_log(Typed *v, small indent, small depth) {
	if (indent == depth) {
		tab(indent);
		util_typelog(v);
		return;
	}
	// if (!v) printf("\x1b[93m(null)\x1b[0m\n");
	switch (v->type) {
		case STRING_LITERAL: {
			StringLiteral *ltr = (StringLiteral *)v;
			printf("\x1b[92m\"%s\"\x1b[0m\n", ltr->value);
			break;
		} case LITERAL: {
			Literal *ltr = (Literal *)v;
			printf("\x1b[93m%s\x1b[0m\n", ltr->value);
			break;
		} case IDENTIFIER: {
			Identifier *id = (Identifier *)v;
			id->kind
				? printf("\x1b[94;3m%s\x1b[0m <\x1b[34;1m%s\x1b[0m>\n", id->name, id->kind)
				: printf("\x1b[94;3m%s\x1b[0m\n", id->name);
			break;
		} case PROGRAM: {
			printf("\x1b[95mProgram\x1b[0m {\n");
			tab(indent);
			for (int i = 0; i < ((Program *)v)->body->l; i++) {
				tab(indent+1);
				util_log(((Program *)v)->body->a[i], indent+1, depth);
			};
			printf("}\n");
			break;
		} case IMPORT_DECLARATION: {
			printf("\x1b[95mImportDeclaration\x1b[0m {\n");
			ImportDeclaration *decl = (ImportDeclaration *)v;
			tab(indent+1); printf("\x1b[96msource\x1b[0m: ");
			util_linelog((Typed *)decl->source);
			for (int i = 0; i < decl->specifiers->l; i++) {
				util_log(decl->specifiers->a[i], indent+1, depth);
			};
			tab(indent); printf("}\n");
			break;
		}  case FUNCTION_DECLARATION: {
			FunctionDeclaration *decl = (FunctionDeclaration *)v;
			printf("%s\x1b[95mFunctionDeclaration\x1b[0m {\n",
				decl->async ? "\x1b[95;4mAsync\x1b[0m" : "");
			tab(indent+1);
			printf("\x1b[96mname\x1b[0m: ");
			util_linelog((Typed *)decl->name);
			if (decl->body) for (int i = 0; i < decl->body->l; i++) {
				tab(indent+1);
				util_log(decl->body->a[i], indent+1, depth);
			};
			tab(indent); printf("}\n");
			break;
		} case IMPORT_SPECIFIER: {
			tab(indent);
			printf("\x1b[95mImportSpecifier\x1b[0m {\n");
			ImportSpecifier *spec = (ImportSpecifier *)v;
			tab(indent+1);
			printf("\x1b[96mname\x1b[0m: ");
			util_linelog((Typed *)spec->name);
			tab(indent+1);
			printf("\x1b[96mlocal\x1b[0m: ");
			util_linelog((Typed *)spec->local);
			tab(indent);
			printf("}\n");
			break;
		} case ASSIGNMENT_EXPRESSION: {
			printf("\x1b[95mAssignmentExpression\x1b[0m {\n");
			AssignmentExpression *expr = (AssignmentExpression *)v;
			tab(indent+1);
			printf("\x1b[96mleft\x1b[0m: ");
			util_linelog((Typed *)expr->left);
			tab(indent+1);
			printf("\x1b[96mright\x1b[0m: ");
			util_log((Typed *)expr->right, indent+1, depth);
			tab(indent);
			printf("}\n");
			break;
		}  default: printf("\x1b[93mUnknown %i\x1b[0m\n", v->type);
	}
}

void printAST(Program *p, small depth) {
	util_log((Typed *)p, 0, depth);
	printf("\n");
}

void codeError(Error *e) {
	string white = "\x1b[90m";
	string red = "\x1b[91m";
	string yellow = "\x1b[93m";
	string blue = "\x1b[94;3m";
	string clear = "\x1b[0m";
	string bold = "\x1b[1m";
	FILE *fp = fopen(e->file, "r");
	string line = NULL;
	size_t len = 0;
	size_t read = 0;
	int l = 0;
	if (fp == NULL) {
		printf("weird\n");
		return;
	}
	struct winsize *w = malloc(sizeof(struct winsize));
	ioctl(STDOUT_FILENO, TIOCGWINSZ, w);
	int lnl = intl(e->token->pos.line+1);
	size_t errlen = strlen(e->file) + strlen(e->name) + strlen(e->data) + 14 + lnl;
	if (errlen < w->ws_col) {
		printf("%s%s", clear, white);
		repeat("─", intl(e->token->pos.line+1) + 2);
		// repeat("-", intl(e->token->pos.line+1) + 2);
		printf("┬─%s%s%s %s %s%s─%s%s %s%s:%s%i%s:%s%i %s%s─%s%s %s %s%s",
		// printf("+-%s%s%s %s %s%s-%s%s %s%s:%s%i%s:%s%i %s%s─%s%s %s %s%s",
			white, bold, red, e->name, clear,
			white, bold, blue, e->file, clear,
			yellow, e->token->pos.line, clear,
			yellow, e->token->pos.start, clear,
			white, clear, bold, e->data, clear, white
		);
		repeat("─", w->ws_col - errlen - intl(e->token->pos.line) - intl(e->token->pos.start));
		// repeat("-", w->ws_col - errlen - intl(e->token->pos.line) - intl(e->token->pos.start));
		printf("%s", clear);
	}
	printf("\n");
	while ((read = getline(&line, &len, fp)) != -1UL) {
		++l;
		if (l == e->token->pos.line-1 || l == e->token->pos.line+1) {
			printf("%s ", white);
			repeat(" ", lnl - intl(l));
			printf("%i │%s %s", l, clear, line);
			// printf("%i |%s %s", l, clear, line);
		} else if (l == e->token->pos.line) {
			int lens[3] = {
				e->token->pos.start-1,
				e->token->pos.absEnd - e->token->pos.absStart,
				strlen(line)-e->token->pos.end
			};
			string start = substr(line, 0, lens[0]);
			string error = substr(line, lens[0], lens[1]);
			string end = substr(line, lens[0]+lens[1], lens[2]);
			printf("%s ", white);
			repeat(" ", lnl - intl(l));
			printf("%i │%s %s%s%s%s%s\n", l, clear, start, red, error, clear, end);
			// printf("%i |%s %s%s%s%s%s\n", l, clear, start, red, error, clear, end);
			free(start); free(error); free(end);
		}
	}
	if (w->ws_col >= 5) {
		printf("%s", white);
		repeat("─", intl(e->token->pos.line+1)+2);
		// repeat("-", intl(e->token->pos.line+1)+2);
		printf("┴");
		// printf("+");
		repeat("─", w->ws_col - 1 - intl(e->token->pos.line+1)-2);
		// repeat("-", w->ws_col - 1 - intl(e->token->pos.line+1)-2);
		printf("%s\n", clear);
	}
	free(w);
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
	}
}
