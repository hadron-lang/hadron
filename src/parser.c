#include "parser.h"

#include "types.h"
#include "util/array.h"
#include "util/stack.h"
#include "util/str.h"
#include <stdio.h>

static void initParser(char *, Array *, char *);
static boolean end(void);
static boolean start(void);
static Token *next(void);
static Token *prev(void);
static Token *peekNext(void);
static Token *current(void);
static boolean match(Type);
static boolean nextIs(Type);
static boolean check(Value *);
static boolean matchAny(int, ...);
static Value *fmatch(Type, char *);
static Value *omatch(Type);

static void *nop(void *);
static Value *toValue(void *);

static void delmt(boolean throw);

static Value *stmt(void);
static Value *forStmt(void);
static Value *whileStmt(void);
static Value *ifStmt(void);
static Value *exprStmt(void);
static Value *retStmt(void);

static Value *decl(void);
static Value *impDecl(void);
static Value *impSpec(void);
static Value *impSpecArray(void);
static Value *funcDecl(void);
static Value *classDecl(void);
static Value *asgnExpr(Value *);
static Value *callExpr(Value *);
static Value *expr(void);
static Value *identifier(Value *);
static Value *literal(void);
static Value *strLiteral(void);

static struct Parser {
	int iterator;
	Array *tokens;
	Program *program;
	Array *errors;
	char *code;
	char *file;
} parser;

void initParser(char *code, Array *tokens, char *file) {
	parser.tokens = tokens;
	parser.file = file;
	parser.program = initProgram(tokens->l/4);
	parser.errors = newArray(2);
	parser.iterator = -1;
	parser.code = code;
}

void *nop(void *x) { if (x) free(x); return NULL; }

Value *fmatch(Type t, char *e) {
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

boolean end() { return parser.iterator > parser.tokens->l-3; }

boolean start() { return parser.iterator < 1; }

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

boolean matchAny(int n, ...) {
	va_list v; va_start(v, n);
	for (int i = 0; i < n; i++) {
		if (match(va_arg(v, int))) { va_end(v); return true; }
	}; va_end(v); return false;
}

boolean match(Type type) {
	if (!peekNext()) return false;
	if (peekNext()->type == type) { next(); return true; }
	else return false;
}

boolean nextIs(Type type) {
	return peekNext() && peekNext()->type == type;
}

boolean check(Value *r) {
	if (!r) return true;
	else if (r->error) {
		pushArray(parser.errors, r->value);
		return true;
	} else return false;
}

void delmt(boolean throw) {
	match(DELMT);
	if (throw && current() && peekNext() &&
		(current()->pos.line == peekNext()->pos.line)
	) pushArray(parser.errors, error("Parse", parser.file, "expected delimiter", current()));
}

Value *classDecl() {
	Value *n = identifier(NULL);
	if (check(n)) return nop(n);
	if (match(COLON)) {
		Value *p = identifier(NULL);
		if (check(p)) return nop(n), nop(p);
	}
	return NULL;
	// return initClassDeclaration(initIdentifier(
	// 	getTokenContent(parser.code, n->value)
	// ), NULL);
}

Value *funcParam() {
	Value *a = fmatch(NAME, "expected identifier");
	if (check(a)) return nop(a);
	Value *b = omatch(NAME);
	return toValue(initTypedIdentifier(
		getTokenContent(parser.code, !b ? a->value : b->value),
		getTokenContent(parser.code, b ? a->value : NULL)
	));
}

Value *funcParams() {
	Array *a = newArray(2);
	if (match(RPAREN)) return toValue(a);
	Value *f = funcParam();
	if (check(f)) return freeArray(a), nop(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = funcParam();
		if (check(s)) return freeArray(a), nop(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch(RPAREN, "expected )");
	if (check(e)) return freeArray(a), nop(e);
	trimArray(a);
	return toValue(a);
}

Value *funcBody() {
	Array *a = newArray(2);
	while (!match(RCURLY)) {
		Value *v = expr();
		if (check(v)) return freeArray(a), nop(v);
		pushArray(a, v->value);
	};
	trimArray(a);
	return toValue(a);
}

Value *funcDecl() {
	boolean a = false;
	if (match(ASYNC)) a = true;
	Value *f = fmatch(FUNC, "expected `func`");
	if (check(f)) return nop(f);
	free(f);
	// Value *n = identifier(NULL, false);
	// if (!n) return pushArray(parser.errors, error("Parse", parser.file, "wtf, no identifier", current())), NULL;
	Value *n = fmatch(NAME, "expected identifier");
	if (check(n)) return nop(n);
	Value *lp = omatch(LPAREN);
	Value *params = NULL;
	if (lp) params = funcParams();
	free(lp);
	Value *b = fmatch(LCURLY, "expected {");
	if (check(b)) return nop(n), nop(params);
	if (match(RCURLY)) return toValue(initFunctionDeclaration(a,
		initIdentifier(getTokenContent(parser.code, n->value)),
		params ? params->value : NULL, NULL));
	free(b);
	Value *body = funcBody();
	if (check(body)) return nop(body), nop(n), nop(params);
	return toValue(initFunctionDeclaration(a,
		initIdentifier(getTokenContent(parser.code, n->value)),
	params ? params->value : NULL, body ? body->value : NULL));
}

Value *asgnExpr(Value *t) {
	prev();
	Value *n = identifier(t);
	if (check(n)) return nop(t), nop(n);
	Value *q = fmatch(EQ, "expected =");
	if (check(q)) return nop(t), nop(n), nop(q);
	Value *r = expr();
	if (check(r)) return nop(t), nop(n), nop(q), nop(r);
	match(DELMT);
	return toValue(initAssignmentExpression(n->value, r->value));
}

Value *identifier(Value *t) {
	Value *id = fmatch(NAME, "expected identifier");
	if (check(id)) return nop(id), nop(t);
	return toValue(t ? (void *)initTypedIdentifier(
		getTokenContent(parser.code, id->value),
		getTokenContent(parser.code, t->value)
	) : (void *)initIdentifier(
		getTokenContent(parser.code, id->value)
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
Value *varDecl(Value *t) {
	Value *n = identifier(t);
	if (check(n)) return nop(t), nop(n);
	Value *q = fmatch(EQ, "expected =");
	if (check(q)) return nop(t), nop(n), nop(q);
	Value *r = expr();
	if (check(r)) return nop(t), nop(n), nop(q), nop(r);
	match(DELMT);
	return toValue(initAssignmentExpression(n->value, r->value));
}


//* working as expected
Value *callExpr(Value *name) {
	debug_log("callExpr", parser.code, peekNext());
	Array *a = newArray(2);
	// if (match(RPAREN)) return toValue(initCallExpression(
	// 	initIdentifier(getTokenContent(parser.code, name->value)), NULL
	// ));
	match(LPAREN);
	// printf("Current: %i\nNext: %i\n", current()->type, peekNext()->type);
	Value *e = expr();
	// if (check(e)) return freeArray(a), nop(e);
	pushArray(a, e->value);
	Value *r = fmatch(RPAREN, "expected )");
	if (check(r)) return freeArray(a), nop(e), nop(r);
	// while (match(SEP)) {
	// 	Value *e = expr();
	// 	if (check(e)) return freeArray(a), nop(e);
	// 	pushArray(a, e->value);
	// }

	return toValue(initCallExpression(
		initIdentifier(getTokenContent(parser.code, name->value)),
		a
	));
}

Value *binaryExpr(Value *a) {
	BinaryOperator oper = BIN_UNDEF;
	switch (current()->type) {
		case ADD: oper = BIN_ADD; break;
		case SUB: oper = BIN_SUB; break;
		case MUL: oper = BIN_MUL; break;
		case DIV: oper = BIN_DIV; break;
		case LOR: oper = BIN_LOR; break;
		case BOR: oper = BIN_BOR; break;
		case REM: oper = BIN_REM; break;
		case POW: oper = BIN_POW; break;
		case LAND: oper = BIN_LAND; break;
		case BAND: oper = BIN_BAND; break;
		case BXOR: oper = BIN_BXOR; break;
		case RSHIFT: oper = BIN_RSHIFT; break;
		case LSHIFT: oper = BIN_RSHIFT; break;
		default: oper = BIN_UNDEF; break;
	}
	if (oper == BIN_UNDEF) pushArray(parser.errors, error("Parse", parser.file, "internal error", current()));
	Value *b = expr();
	if (check(b)) return nop(a);
	return toValue(initBinaryExpression(oper,
		(Typed *)initLiteral(getTokenContent(parser.code, a->value)), b->value
	));
}

Value *expr() {
	debug_log("expr", parser.code, peekNext());
	if (match(NAME)) {
		Value *t = toValue(current());
		if (nextIs(NAME)) return varDecl(t);
		else if (nextIs(LPAREN)) return callExpr(t);
		// else if (nextIs(DOT)) return chainExpr();
		else if (nextIs(EQ)) return asgnExpr(t);
		else return prev(), identifier(t);
	} else if (matchAny(4, DEC, HEX, OCTAL, BIN)) {
		Value *l = toValue(current());
		if (matchAny(13,
			ADD, SUB, MUL, DIV, LAND, LOR, BAND, BOR, BXOR, REM, RSHIFT, LSHIFT, POW
		)) return binaryExpr(l);
		printf("< expr returning literal \x1b[93m%s\x1b[0m\n", getTokenContent(parser.code, current()));
		return nop(l), toValue(initLiteral(getTokenContent(parser.code, current())));
	} else if (match(STR)) {
		Value *l = toValue(current());
		printf("< expr returning string literal \x1b[92m\"%s\"\x1b[0m\n", getTokenContent(parser.code, current()));

		// todo add binary expression

		return nop(l), strLiteral();
	}
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
	if (match(RCURLY)) return toValue(a);
	Value *f = impSpec();
	if (check(f)) return freeArray(a), nop(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = impSpec();
		if (check(s)) return freeArray(a), nop(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch(RCURLY, "expected }");
	if (check(e)) return freeArray(a), nop(e);
	trimArray(a);
	return toValue(a);
}

Value *impDecl() {
	Value *n = fmatch(STR, "unexpected token (1)");
	if (check(n)) return nop(n);
	Value *i = fmatch(IMPORT, "unexpected token (2)");
	if (check(i)) return nop(i), nop(n);
	if (match(LCURLY)) {
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

Value *exprStmt() {
	debug_log("exprStmt", parser.code, peekNext());
	Value *e = expr();
	if (check(e)) return nop(e);
	// if (check(e)) {
	// 	printf("test: %p\n", e);
	// 	pushArray(parser.errors, error("Parse", parser.file, "no expression", current()));
	// 	return nop(e);
	// }
	delmt(false);

	Value *v = toValue(initExpressionStatement(e->value));

	return v;
}

Value *ifStmt() {
	// Value *condition = expr();
	return NULL;
} // todo

Value *whileStmt() {
	return NULL;
} // todo

Value *forStmt() {
	return NULL;
} // todo

Value *retStmt() {
	Value *retval = expr();
	return retval;
} // todo

Value *stmt() {
	debug_log("stmt", parser.code, peekNext());
	if (match(FOR)) return forStmt();
	else if (match(WHILE)) return whileStmt();
	else if (match(RETURN)) return retStmt();
	else if (match(IF)) return ifStmt();
	return exprStmt();
}

Value *decl() {
	debug_log("decl", parser.code, peekNext());
	if (match(FROM)) return impDecl();
	if (match(CLASS)) return classDecl();
	if (nextIs(FUNC) || nextIs(ASYNC)) return funcDecl();
	return stmt();
}

void synchronize() {
	debug_log("synchronize", parser.code, peekNext());
	while (!end()) {
		if (peekNext()) switch (peekNext()->type) {
			case CLASS: case IF:
			case FUNC: case FOR:
			case WHILE: case DO:
			case FROM: case RETURN:
			case ASYNC: return;
			case DELMT: next(); return;
			default: next();
		};
	}
}

Result *parse(char *code, Array *tokens, char *fname) {
	printf("\n");
	initParser(code, tokens, fname);
	Result *result = malloc(sizeof(Result));
	while (!end()) {
		printf("> loop\n");
		Value *d = decl();
		if (check(d)) synchronize();
		else {
			printf("< pushing value, type: %i\n", ((Typed *)d->value)->type);
			pushArray(parser.program->body, d->value);
		}
	}
	printf("\n");
	trimArray(parser.errors);

	result->errors = parser.errors;
	result->data = parser.program;

	return result;
}
