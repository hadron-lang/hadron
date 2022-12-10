#include "parser.h"

static void initParser(string, Array *, string);
static bool end(void);
static bool start(void);
static Token *next(void);
static Token *peekNext(void);
static Token *peekPrev(void);
static Token *current(void);
static bool match(Type);

static struct Parser {
	int iterator;
	Array *tokens;
	Program *program;
	Array *errors;
	string code;
	string file;
} parser;

static Res *fmatch(Type, string);
static bool check(Res *);

void initParser(string code, Array *tokens, string file) {
	parser.tokens = tokens;
	parser.file = file;
	parser.program = initProgram(tokens->l/4);
	parser.errors = newArray(2);
	parser.iterator = -1;
	parser.code = code;
	// call to avoid gcc warnings
	current();
	match(0);
	peekPrev();
	fmatch(0, "");
	check(NULL);
}

Res *fmatch(Type t, string e) {
	Res *r = malloc(sizeof(Res));
	if (match(t)) {
		r->type = RTOKEN;
		r->value = current();
	} else {
		r->type = RERROR;
		r->value = error("Parse", parser.file, e, peekNext());
	}; return r;
}

bool end() { return parser.iterator > parser.tokens->l-2; }
bool start() { return parser.iterator < 1; }
Token *next() {
	if (!end()) return parser.tokens->a[++parser.iterator];
	else return NULL;
}
Token *peekNext() {
	if (!end()) return parser.tokens->a[parser.iterator+1];
	else return NULL;
}
Token *peekPrev() {
	if (!start()) return parser.tokens->a[parser.iterator-1];
	else return NULL;
}
Token *current() { return parser.tokens->a[parser.iterator]; }
bool match(Type type) {
	if (peekNext()->type == type) { next(); return true; }
	else return false;
}

bool check(Res *r) {
	if (r == NULL) return (bool)0;
	else if (r->type == RERROR) {
		pushArray(parser.errors, r->value);
		return (bool)1;
	} else return (bool)0;
}

Result *parse(string code, Array *tokens, string fname) {
	initParser(code, tokens, fname);
	Result *result = malloc(sizeof(Result));
	// pushArray(parser.errors, error("Parse", fname, "test error (145th token)", (Token *)parser.tokens->a[145]));
	while (!end()) {
		switch (next()->type) {
			default: {}
		}
	}
	trimArray(parser.errors);
	result->errors = parser.errors;
	result->data = parser.program;
	return result;
}
