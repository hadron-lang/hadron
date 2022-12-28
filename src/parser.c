#include "parser.h"

static void initParser(string, Array *, string);
static bool end(void);
static bool start(void);
static Token *next(void);
static Token *prev(void);
static Token *peekNext(void);
static Token *current(void);
static bool match(Type);
static bool nextIs(Type);
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
static Value *asgnExpr(Value *);
static Value *exprStmt(void);
static Value *identifier(Value *);
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
	omatch(0);
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
Token *prev() {
	if (!start()) return parser.tokens->a[--parser.iterator];
	else return NULL;
}
Token *peekNext() {
	if (!end()) return parser.tokens->a[parser.iterator+1];
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

bool nextIs(Type type) {
	return peekNext() && peekNext()->type == type;
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
Value *funcParam() {
	Value *a = fmatch(NAME, "expected identifier");
	if (check(a)) return nop(a);
	Value *b = omatch(NAME);
	return toValue(initIdentifier(
		getTokenContent(parser.code, !b ? a->value : b->value),
		getTokenContent(parser.code, b ? a->value : NULL)
	));
}
Value *funcParams() {
	Array *a = newArray(2);
	if (match($PAREN)) return toValue(a);
	Value *f = funcParam();
	if (check(f)) return freeArray(a), nop(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = funcParam();
		if (check(s)) return freeArray(a), nop(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch($PAREN, "expected )");
	if (check(e)) return freeArray(a), nop(e);
	trimArray(a);
	return toValue(a);
}
Value *funcBody() {
	Array *a = newArray(2);
	while (!match($CURLY)) {
		Value *v = exprStmt();
		if (check(v)) return freeArray(a), nop(v);
		pushArray(a, v->value);
	};
	trimArray(a);
	return toValue(a);
}
Value *funcDecl() {
	bool a = false;
	if (match(ASYNC)) a = true;
	Value *f = fmatch(FUNC, "expected `func`");
	if (check(f)) return nop(f);
	free(f);
	// Value *n = identifier(NULL, false);
	// if (!n) return pushArray(parser.errors, error("Parse", parser.file, "wtf, no identifier", current())), NULL;
	Value *n = fmatch(NAME, "expected identifier");
	if (check(n)) return nop(n);
	Value *lp = omatch(PAREN);
	Value *params = NULL;
	if (lp) params = funcParams();
	free(lp);
	Value *b = fmatch(CURLY, "expected {");
	if (check(b)) return nop(n), nop(params);
	if (match($CURLY)) return toValue(initFunctionDeclaration(a,
		initIdentifier(getTokenContent(parser.code, n->value), NULL),
		params ? params->value : NULL, NULL));
	free(b);
	Value *body = funcBody();
	if (check(body)) return nop(body), nop(n), nop(params);
	return toValue(initFunctionDeclaration(a,
		initIdentifier(getTokenContent(parser.code, n->value), NULL),
	params ? params->value : NULL, body ? body->value : NULL));
}
Value *asgnExpr(Value *t) {
	prev();
	Value *n = identifier(t);
	if (!n) return nop(t);
	Value *q = fmatch(EQ, "expected =");
	if (check(q)) return nop(t), nop(n), nop(q);
	Value *r = exprStmt();
	if (check(r)) return nop(t), nop(n), nop(q), nop(r);
	match(DELMT);
	return toValue(initAssignmentExpression(n->value, r->value));
}
Value *identifier(Value *t) {
	if (!match(NAME)) return NULL;
	// Value *n = fmatch(NAME, "expected identifier");
	// if (!n && report && check(n)) return nop(n);
	// else if (!n && !report) return nop(n);
	return toValue(initIdentifier(
		getTokenContent(parser.code, current()),
		t ? getTokenContent(parser.code, t->value) : NULL
	));
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
	if (match(NAME)) {
		Value *t = toValue(current());
		if (match(NAME)) return asgnExpr(t);
		else if (nextIs(EQ)) return asgnExpr(nop(t));
		else return prev(), identifier(nop(t));
	};
	return literal();
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
		initIdentifier(getTokenContent(parser.code, n->value), NULL),
		initIdentifier(getTokenContent(parser.code, n->value), NULL)
	));
	Value *ln = fmatch(NAME, "expected identifier");
	if (check(ln)) return nop(ln);
	return toValue(initImportSpecifier(
		initIdentifier(getTokenContent(parser.code, n->value), NULL),
		initIdentifier(getTokenContent(parser.code, ln->value), NULL)
	));
}
Value *impSpecArray() {
	Array *a = newArray(2);
	if (match($CURLY)) return toValue(a);
	Value *f = impSpec();
	if (check(f)) return freeArray(a), nop(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = impSpec();
		if (check(s)) return freeArray(a), nop(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch($CURLY, "expected }");
	if (check(e)) return freeArray(a), nop(e);
	trimArray(a);
	return toValue(a);
}
Value *impDecl() {
	Value *n = fmatch(STR, "unexpected token (1)");
	if (check(n)) return nop(n);
	Value *i = fmatch(IMPORT, "unexpected token (2)");
	if (check(i)) return nop(i), nop(n);
	if (match(CURLY)) {
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
	if (nextIs(FUNC) || nextIs(ASYNC)) return funcDecl();
	return exprStmt();
}

void synchronize() {
	next();
	while (!end()) {
		if (current() && current()->type == DELMT) return;
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
