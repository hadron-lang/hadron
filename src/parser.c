#include "parser.h"

static void initParser(string, Array *, string);
static bool end(void);
static bool start(void);
static Token *next(void);
static Token *peekNext(void);
static Token *peekPrev(void);
static Token *current(void);
static bool match(Type);
static bool check(Value *);
static bool matchAny(small, ...);
static Value *fmatch(Type, string);
static Value *omatch(Type);

static void *nop(void *);
static Value *toValue(void *);
static Value *decl(void);
static Value *impDecl(void);
static Value *impSpec(void);
static Value *impSpecArray(void);
static Value *funcDecl(void);
static Value *classDecl(void);
static Value *asgnExpr(void);
static Value *exprStmt(void);
static Value *identifier(void);
static Value *literal(void);
static Value *strLiteral(void);

static struct Parser {
	int iterator;
	Array *tokens;
	Program *program;
	Array *errors;
	string code;
	string file;
} parser;

void initParser(string code, Array *tokens, string file) {
	parser.tokens = tokens;
	parser.file = file;
	parser.program = initProgram(tokens->l/4);
	parser.errors = newArray(2);
	parser.iterator = -1;
	parser.code = code;
	// call to avoid gcc warnings
	peekPrev();
	matchAny(0);
}

void *nop(void *x) { if (x) free(x); return NULL; }

Value *fmatch(Type t, string e) {
	Value *r = malloc(sizeof(Value));
	if (match(t)) {
		r->error = false;
		r->value = current();
	} else {
		r->error = true;
		r->value = error("Parse", parser.file, e, next());
	}; return r;
}
Value *omatch(Type t) {
	if (match(t)) return toValue(current());
	else return NULL;
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
Token *current() {
	return parser.tokens->a[parser.iterator];
}

bool matchAny(small n, ...) {
	va_list v; va_start(v, n);
	for (small i = 0; i < n; i++) {
		if (match(va_arg(v, int))) { va_end(v); return true; }
	}; va_end(v); return false;
}

bool match(Type type) {
	if (!peekNext()) return false;
	if (peekNext()->type == type) { next(); return true; }
	else return false;
}

bool check(Value *r) {
	if (!r) return true;
	else if (r->error) {
		pushArray(parser.errors, r->value);
		return true;
	} else return false;
}

Value *classDecl() {
	Value *n = fmatch(NAME, "unexpected token (6)");
	if (check(n)) return nop(n);
	return n;
}
Value *funcDecl() {
	Value *n = fmatch(NAME, "unexpected token (5)");
	if (check(n)) return nop(n);
	return n;
}
Value *asgnExpr() {
	Value *t = identifier();
	if (check(t)) return nop(t);
	Value *l = omatch(NAME);
	Value *q = fmatch(EQ, "expected =");
	if (check(q)) return nop(t), nop(l), nop(q);
	Value *r = exprStmt();
	if (check(r)) return nop(t), nop(l), nop(q), nop(r);
	match(DELMT);
	return toValue(initAssignmentExpression(
		(Typed *)initIdentifier(getTokenContent(parser.code, l ? l->value : t->value)),
		r->value
	));
}
Value *identifier() {
	Value *n = fmatch(NAME, "expected identifier");
	if (check(n)) return nop(n);
	return n;
}
Value *strLiteral() {
	return toValue(initStringLiteral(
		getTokenContent(parser.code, current())
	));
}
Value *literal() {
	if (match(STR)) return strLiteral();
	if (!matchAny(6, TRUE, FALSE, DEC, HEX, OCTAL, BIN)) return NULL;
	return toValue(initLiteral(getTokenContent(parser.code, current())));
}
Value *exprStmt() {
	// printf("\nentered exprStmt:\n%s\n\n", getTokenContent(parser.code, next()));
	if (peekNext() && peekNext()->type == NAME) return asgnExpr();
	return literal();
	next();
	return NULL;
}
Value *toValue(void *x) {
	if (!x) return NULL;
	Value *r = malloc(sizeof(Value));
	r->error = false;
	r->value = x;
	return r;
}
Value *impSpec() {
	Value *n = fmatch(NAME, "unexpected token (9)");
	if (check(n)) return nop(n);
	if (!match(AS)) return toValue(initImportSpecifier(
		initIdentifier(getTokenContent(parser.code, n->value)),
		initIdentifier(getTokenContent(parser.code, n->value))
	));
	Value *ln = fmatch(NAME, "expected identifier");
	if (check(ln)) return nop(ln);
	return toValue(initImportSpecifier(
		initIdentifier(getTokenContent(parser.code, n->value)),
		initIdentifier(getTokenContent(parser.code, ln->value))
	));
}
Value *impSpecArray() {
	Array *a = newArray(2);
	Value *f = impSpec();
	if (check(f)) return freeArray(a), nop(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = impSpec();
		if (check(s)) return nop(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch(CBRACKET, "unexpected token bruh");
	if (check(e)) return freeArray(a), nop(f), nop(e);
	trimArray(a);
	return toValue(a);
}
Value *impDecl() {
	Value *n = fmatch(STR, "unexpected token (1)");
	if (check(n)) return nop(n);
	Value *i = fmatch(IMPORT, "unexpected token (2)");
	if (check(i)) return nop(i), nop(n);
	if (match(CBRACKET)) {
		Value *speca = impSpecArray();
		if (check(speca)) return nop(speca), nop(n), nop(i);
		match(DELMT);
		return toValue(initImportDeclaration(
			initStringLiteral(
				getTokenContent(parser.code, n->value)
			), speca->value)
		);
	} else {
		Value *spec = impSpec();
		if (check(spec)) return nop(spec), nop(n), nop(i);
		Array *speca = newArray(1);
		pushArray(speca, spec->value);
		match(DELMT);
		return toValue(initImportDeclaration(
			initStringLiteral(
				getTokenContent(parser.code, n->value)
			), speca
		));
	}
}
Value *decl() {
	// Value *r = malloc(sizeof(Value));
	if (match(FROM)) return impDecl();
	if (match(CLASS)) return classDecl();
	if (match(FUNC)) return funcDecl();
	return exprStmt();
}

void synchronize() {
	next();
	while (!end()) {
		if (current() && (current()->type == DELMT
			|| current()->type == NEWLINE)) return;
		if (peekNext()) switch (peekNext()->type) {
			case CLASS: case IF:
			case FUNC: case FOR:
			case WHILE: case DO:
			case RET: return;
			default: break;
		}; next();
	}
}

Result *parse(string code, Array *tokens, string fname) {
	initParser(code, tokens, fname);
	Result *result = malloc(sizeof(Result));
	// pushArray(parser.errors, error("Parse", fname, "test error (145th token)", (Token *)parser.tokens->a[145]));
	while (!end()) {
		Value *d = decl();
		if (check(d)) synchronize();
		else pushArray(parser.program->body, d->value);
	}
	trimArray(parser.errors);
	result->errors = parser.errors;
	result->data = parser.program;
	return result;
}
