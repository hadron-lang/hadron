#include "parser.h"

static void initParser(string, Array *);
static bool end(void);
static bool start(void);
static Token *next(void);
static Token *peekNext(void);
static Token *peekPrev(void);
static Token *current(void);
static bool match(Type);

struct Parser {
	int iterator;
	Array *tokens;
	Program *program;
	Array *errors;
	string code;
} parser;

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

extern Result *parse(string code, Array *tokens) {
	initParser(code, tokens);
	Result *result = malloc(sizeof(Result));
	result->errors = parser.errors;
	result->data = parser.program;
	return result;
};
