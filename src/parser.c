#include "parser.h"

static void initParser(string, Array *);
static bool end(void);
// static bool start(void);
static Token *next(void);
static Token *peekNext(void);
// static Token *peekPrev(void);
static Token *current(void);
static bool match(Type);

Parser parser;

void initParser(string code, Array *tokens) {
	parser.tokens = tokens;
	Program *program = initProgram(tokens->l/4);
	parser.program = program;
	Array *errors = malloc(sizeof(Array));
	initArray(errors, 2);
	parser.errors = errors;
	parser.iterator = -1;
	parser.code = code;
};

ImportDeclaration *walk_import(string src) {
	Array *speca = malloc(sizeof(Array));
	initArray(speca, 2);
	ImportSpecifier *spec;
	while (next()->type != CBRACKET) {
		string name;
		string local;
		if (current()->type == NAME) {
			name = malloc(current()->end - current()->start + 1);
			memcpy(name, parser.code+current()->start, current()->end - current()->start);
			if (match(AS)) {
				if (match(NAME)) {
					local = malloc(current()->end - current()->start + 1);
					memcpy(local, parser.code+current()->start, current()->end - current()->start);
					spec = initImportSpecifier(name, local);
					pushArray(speca, spec);
				} else pushArray(parser.errors, error("Parse", "temp/index.idk", -1, 1, "2"));
			} else if (match(SEP) || match(CBRACKET)) {
				local = name;
				spec = initImportSpecifier(name, local);
				pushArray(speca, spec);
			} else pushArray(parser.errors, error(
				"Parse", "temp/index.idk", -1, 2,
				"(1) got ",
				getType(peekNext()->type, "")
			));
		} else pushArray(parser.errors, error(
			"Parse", "temp/index.idk", -1, 2,
			"(0) got ",
			getType(current()->type, "")
		));
	}
	trimArray(speca);
	ImportDeclaration *decl = initImportDeclaration(src, speca);
	return decl;
}
// static Typed *walk(void) {
// 	return NULL;
// }

bool end() { return parser.iterator > parser.tokens->l-2; }
// bool start() { return parser.iterator < 1; }
Token *next() {
	if (!end()) return parser.tokens->a[++parser.iterator];
	else return NULL;
}
Token *peekNext() {
	if (!end()) return parser.tokens->a[parser.iterator+1];
	else return NULL;
}
// Token *peekPrev() {
// 	if (!start()) return parser.tokens->a[parser.iterator-1];
// 	else return NULL;
// }
Token *current() { return parser.tokens->a[parser.iterator]; }
bool match(Type type) {
	if (peekNext()->type == type) { next(); return true; }
	else return false;
}

extern Result *parse(string code, Array *tokens) {
	initParser(code, tokens);
	Token *t;
	while (!end()) {
		t = next();
		if (t->type == FROM) {
			Token *f = next();
			string src;
			Array *speca = malloc(sizeof(Array));
			initArray(speca, 1);
			if (f->type == STR) {
				src = malloc(f->end - f->start - 1);
				memcpy(src, parser.code+f->start+1, f->end - f->start-2);
			} else {
				pushArray(parser.errors, error(
					"Parse", "temp/index.idk", -1, 4,
					"expected ",
					getType(STR, "\x1b[93m"),
					", got ",
					getType(f->type, "\x1b[93m")
				));
			}
			if (next()->type != IMPORT) pushArray(parser.errors, error("Parse", "temp/index.idk", -1, 1, "expected \x1b[96mimport"));
			if (next()->type == CBRACKET) {
				ImportDeclaration *decl = walk_import(src);
				pushArray(parser.program->body, decl);
			} else if (current()->type == NAME) {
				string local = malloc(current()->end - current()->start + 1);
				memcpy(local, parser.code+current()->start, current()->end - current()->start);
				ImportSpecifier *spec = initImportSpecifier("exports", local);
				pushArray(speca, spec);
				ImportDeclaration *decl = initImportDeclaration(src, speca);
				pushArray(parser.program->body, decl);
			}
		}
	};

	Result *result = malloc(sizeof(Result));
	result->errors = parser.errors;
	result->data = parser.program;
	return result;
};
