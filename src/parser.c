#include "parser.h"

Parser parser;

void initParser(string code, TArray *tokens) {
	parser.tokens = tokens;
	Program *program = initProgram(tokens->length/4);
	parser.program = program;
	Array *errors = malloc(sizeof(Array));
	initArray(errors, 2);
	parser.errors = errors;
	parser.iterator = -1;
	parser.code = code;
};

Typed *walk(void) {
	return NULL;
}

bool temp_end() { return parser.iterator > parser.tokens->length-2; }
bool temp_start() { return parser.iterator < 1; }
Token *temp_next() {
	if (!temp_end()) return parser.tokens->array[++parser.iterator];
	else return NULL;
}
Token *temp_peekNext() {
	if (!temp_end()) return parser.tokens->array[parser.iterator+1];
	else return NULL;
}
Token *temp_peekPrev() {
	if (!temp_start()) return parser.tokens->array[parser.iterator-1];
	else return NULL;
}
Token *temp_current() { return parser.tokens->array[parser.iterator]; }
bool temp_match(Type type) {
	if (temp_peekNext()->type == type) { temp_next(); return true; }
	else return false;
}

Result *parse(string code, TArray *tokens) {
	initParser(code, tokens);
	// Token t;
	// while (!end()) {
		// t = next();
	// }
	Token *t;
	while (!temp_end()) {
		t = temp_next();
		if (t->type == FROM) {
			Token *f = temp_next();
			string src;
			ImportSpecifierArray *speca = malloc(sizeof(ImportSpecifierArray));
			initArray(speca, 1);
			if (f->type == STR) {
				src = malloc(f->end - f->start - 1);
				memcpy(src, parser.code+f->start+1, f->end - f->start-2);
			} else {
				push(parser.errors, Error(
					"Parse", 4,
					"expected ",
					getType(STR, "\x1b[93m"),
					", got ",
					getType(f->type, "\x1b[93m")
				));
			}
			if (temp_next()->type != IMPORT) push(parser.errors, Error("Parse", 1, "import"));
			if (temp_next()->type == CBRACKET) {
				while (temp_peekNext()->type != CBRACKET) {
					temp_next();
				}
			} else push(parser.errors, Error("Parse", 1, "{"));
			ImportSpecifier *spec = initImportSpecifier("a", "b");
			push(speca, spec);
			ImportDeclaration *decl = initImportDeclaration(src, speca);
			push(&parser.program->body, decl);
		}
	};

	// ImportSpecifier *spec0 = initImportSpecifier("get", "httpGET");
	// ImportSpecifier *spec1 = initImportSpecifier("post", "httpPOST");
	// ImportSpecifierArray *speca = malloc(sizeof(ImportSpecifierArray));
	// initArray(speca, 2);
	// push(speca, spec0);
	// push(speca, spec1);
	// ImportDeclaration *decl = initImportDeclaration("http", speca);
	// push(&parser.program->body, decl);

	Result *result = malloc(sizeof(Result));
	result->errors = parser.errors;
	result->data = parser.program;
	return result;
};
