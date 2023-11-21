#include "parser.h"

#include "types.h"
#include "util/array.h"
#include "util/stack.h"
#include "util/str.h"
#include <stdio.h>

static void	   initParser(char *, Array *, char *);
static void    synchronize();
// go to nearest delimiter or newline
static void    psync();
static boolean end(void);
static boolean start(void);
static Token  *next(void);
static Token  *prev(void);
static Token  *peekNext(void);
static Token  *current(void);
static boolean match(Type);
static boolean nextIs(Type);
static boolean check(Value *);
static boolean matchAny(int, ...);
static Value  *fmatch(Type, char *);
static Value  *omatch(Type);

static void	 *freeValue(void *);
static Value *toValue(void *);

static void delmt(boolean throw);

static Value *parseStatement(void);
static Value *parseForStatement(void);
static Value *parseWhileStatement(void);
static Value *parseIfStatement(void);
static Value *parseExpressionStatement(void);
static Value *parseReturnStatement(void);

static Value *parseDeclaration(void);
static Value *parseImportDeclaration(void);
static Value *parseImportSpecifier(void);
static Value *impSpecArray(void);
static Value *parseFunctionDeclaration(void);
static Value *parseClassDeclaration(void);
static Value *parseAssignmentExpression(Value *);
static Value *parseCallExpression(Value *);
static Value *parseExpression(void);
static Value *parseIdentifier(Value *);
static Value *parseLiteral(void);
static Value *parseStringLiteral(void);

static struct Parser {
	int		 iterator;
	Array	*tokens;
	Program *program;
	Array	*errors;
	char	*code;
	char	*file;
} parser;

void initParser(char *code, Array *tokens, char *file) {
	parser.tokens	= tokens;
	parser.file		= file;
	parser.program	= initProgram(tokens->l / 4);
	parser.errors	= newArray(2);
	parser.iterator = -1;
	parser.code		= code;
}

void *freeValue(void *x) {
	if (x) free(x);
	return NULL;
}

Value *fmatch(Type t, char *e) {
	Value *r = malloc(sizeof(Value));
	if (match(t)) {
		r->error = false;
		r->value = current();
	} else {
		r->error = true;
		r->value = error("Parse", parser.file, e, next());
	}
	return r;
}

Value *omatch(Type t) {
	if (match(t)) return toValue(current());
	else
		return NULL;
}

boolean end() { return parser.iterator > parser.tokens->l - 3; }

boolean start() { return parser.iterator < 1; }

Token *next() {
	if (!end()) return parser.tokens->a[++parser.iterator];
	else
		return NULL;
}

Token *prev() {
	if (!start()) return parser.tokens->a[--parser.iterator];
	else
		return NULL;
}

Token *peekNext() {
	if (!end()) return parser.tokens->a[parser.iterator + 1];
	else
		return NULL;
}

Token *current() { return parser.tokens->a[parser.iterator]; }

boolean matchAny(int n, ...) {
	va_list v;
	va_start(v, n);
	for (int i = 0; i < n; i++) {
		if (match(va_arg(v, int))) {
			va_end(v);
			return true;
		}
	};
	va_end(v);
	return false;
}

boolean match(Type type) {
	if (!peekNext()) return false;
	if (peekNext()->type == type) {
		next();
		return true;
	} else
		return false;
}

boolean nextIs(Type type) { return peekNext() && peekNext()->type == type; }

boolean check(Value *r) {
	if (!r) return true;
	else if (r->error) {
		pushArray(parser.errors, r->value);
		return true;
	} else
		return false;
}

void delmt(boolean throw) {
	match(DELMT);
	if (throw && current() && peekNext() &&
		(current()->pos.line == peekNext()->pos.line))
		pushArray(parser.errors,
			error("Parse", parser.file, "expected delimiter", current()));
}

void psync() {
	debug_log("psync", parser.code, peekNext());
	while (!end() && (current()->type != DELMT && current()->pos.line == peekNext()->pos.line)) {
		next();
	}
}

Value *parseClassDeclaration() {
	Value *n = parseIdentifier(NULL);
	if (check(n)) return freeValue(n);
	if (match(COLON)) {
		Value *p = parseIdentifier(NULL);
		if (check(p)) return freeValue(n), freeValue(p);
	}
	return NULL;
	// return initClassDeclaration(initIdentifier(
	// 	getTokenContent(parser.code, n->value)
	// ), NULL);
}

Value *funcParam() {
	Value *a = fmatch(NAME, "expected identifier");
	if (check(a)) return freeValue(a);
	Value *b = omatch(NAME);
	return toValue(initTypedIdentifier(
		getTokenContent(parser.code, !b ? a->value : b->value),
		getTokenContent(parser.code, b ? a->value : NULL)));
}

Value *funcParams() {
	Array *a = newArray(2);
	if (match(RPAREN)) return toValue(a);
	Value *f = funcParam();
	if (check(f)) return freeArray(a), freeValue(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = funcParam();
		if (check(s)) return freeArray(a), freeValue(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch(RPAREN, "expected )");
	if (check(e)) return freeArray(a), freeValue(e);
	trimArray(a);
	return toValue(a);
}

Value *funcBody() {
	Array *a = newArray(2);
	while (!match(RCURLY)) {
		Value *v = parseStatement();
		if (check(v)) {
			psync();
			freeValue(v);
		// if (check(v)) return freeArray(a), freeValue(v);
		} else pushArray(a, v->value);
	};
	trimArray(a);
	return toValue(a);
}

Value *parseFunctionDeclaration() {
	boolean a = false;
	if (match(ASYNC)) a = true;
	Value *f = fmatch(FUNC, "expected `func`");
	if (check(f)) return freeValue(f);
	free(f);
	// Value *n = identifier(NULL, false);
	// if (!n) return pushArray(parser.errors, error("Parse", parser.file, "wtf,
	// no identifier", current())), NULL;
	Value *n = fmatch(NAME, "expected identifier");
	if (check(n)) return freeValue(n);
	Value *lp	  = omatch(LPAREN);
	Value *params = NULL;
	if (lp) params = funcParams();
	free(lp);
	Value *b = fmatch(LCURLY, "expected {");
	if (check(b)) return freeValue(n), freeValue(params);
	if (match(RCURLY))
		return toValue(initFunctionDeclaration(a,
			initIdentifier(getTokenContent(parser.code, n->value)),
			params ? params->value : NULL, NULL));
	free(b);
	Value *body = funcBody();
	if (check(body)) return freeValue(body), freeValue(n), freeValue(params);
	return toValue(initFunctionDeclaration(a,
		initIdentifier(getTokenContent(parser.code, n->value)),
		params ? params->value : NULL, body ? body->value : NULL));
}

Value *parseAssignmentExpression(Value *t) {
	prev();
	Value *n = parseIdentifier(t);
	if (check(n)) return freeValue(t), freeValue(n);
	// Value *q = fmatch(EQ, "expected =");
	next(); // consume operator
	// if (check(q)) return freeValue(t), freeValue(n), freeValue(q);
	Value *r = parseExpression();
	if (check(r)) return freeValue(t), freeValue(n), freeValue(r);
	match(DELMT);
	return toValue(initAssignmentExpression(n->value, r->value, 0));
}

Value *parseIdentifier(Value *t) {
	debug_log("iden", parser.code, peekNext());
	Value *id = fmatch(NAME, "expected identifier");
	if (check(id)) return freeValue(id), freeValue(t);
	return toValue(
		t ? (void *)initTypedIdentifier(getTokenContent(parser.code, id->value),
				getTokenContent(parser.code, t->value))
		  : (void *)initIdentifier(getTokenContent(parser.code, id->value)));
}
Value *parseStringLiteral() {
	return toValue(initStringLiteral(getTokenContent(parser.code, current())));
}
Value *parseLiteral() {
	debug_log("lit", parser.code, peekNext());
	if (match(STR)) return parseStringLiteral();
	if (matchAny(14, FOR, CLASS, FUNC, WHILE, IF, DO, ELSE, FROM, IMPORT, NEW, AWAIT, AS, ASYNC, RETURN)) {
		debug_log("---lit", parser.code, peekNext());
		Value *v = malloc(sizeof(Value));
		v->error = true;
		v->value = error("Parse", parser.file, "unexpected keyword", current());
		return v;
	}
	if (!matchAny(6, TRUE, FALSE, DEC, HEX, OCTAL, BIN)) return NULL;
	return toValue(initLiteral(getTokenContent(parser.code, peekNext())));
}
Value *varDecl(Value *t) {
	Value *n = parseIdentifier(t);
	if (check(n)) return freeValue(t), freeValue(n);
	Value *q = fmatch(EQ, "expected =");
	if (check(q)) return freeValue(t), freeValue(n), freeValue(q);
	Value *r = parseExpression();
	if (check(r)) return freeValue(t), freeValue(n), freeValue(q), freeValue(r);
	match(DELMT);
	return toValue(initAssignmentExpression(n->value, r->value, ASGN_EQ));
}

//* working as expected
Value *parseCallExpression(Value *name) {
	debug_log("callExpr", parser.code, peekNext());
	Array *a = newArray(2);
	// if (match(RPAREN)) return toValue(initCallExpression(
	// 	initIdentifier(getTokenContent(parser.code, name->value)), NULL
	// ));
	match(LPAREN);
	// printf("Current: %i\nNext: %i\n", current()->type, peekNext()->type);
	Value *e = parseExpression();
	// if (check(e)) return freeArray(a), nop(e);
	pushArray(a, e->value);
	Value *r = fmatch(RPAREN, "expected )");
	if (check(r)) return freeArray(a), freeValue(e), freeValue(r);
	// while (match(SEP)) {
	// 	Value *e = expr();
	// 	if (check(e)) return freeArray(a), nop(e);
	// 	pushArray(a, e->value);
	// }

	return toValue(initCallExpression(
		initIdentifier(getTokenContent(parser.code, name->value)), a));
}

Value *binaryExpr(Value *a) {
	BinaryOperator oper = BIN_UNDEF;
	switch (current()->type) {
		case ADD:
			oper = BIN_ADD;
			break;
		case SUB:
			oper = BIN_SUB;
			break;
		case MUL:
			oper = BIN_MUL;
			break;
		case DIV:
			oper = BIN_DIV;
			break;
		case LOR:
			oper = BIN_LOR;
			break;
		case BOR:
			oper = BIN_BOR;
			break;
		case REM:
			oper = BIN_REM;
			break;
		case POW:
			oper = BIN_POW;
			break;
		case LAND:
			oper = BIN_LAND;
			break;
		case BAND:
			oper = BIN_BAND;
			break;
		case BXOR:
			oper = BIN_BXOR;
			break;
		case RSHIFT:
			oper = BIN_RSHIFT;
			break;
		case LSHIFT:
			oper = BIN_RSHIFT;
			break;
		default:
			oper = BIN_UNDEF;
			break;
	}
	if (oper == BIN_UNDEF)
		pushArray(parser.errors,
			error("Parse", parser.file, "internal error", current()));
	Value *b = parseExpression();
	if (check(b)) return freeValue(a);
	return toValue(initBinaryExpression(oper,
		(Typed *)initLiteral(getTokenContent(parser.code, a->value)),
		b->value));
}

Value *parseExpression() {
	debug_log("expr", parser.code, peekNext());
	if (match(NAME)) {
		Value *t = toValue(current());
		if (nextIs(NAME)) return varDecl(t);
		else if (nextIs(LPAREN))
			return parseCallExpression(t);
		// else if (nextIs(DOT)) return chainExpr();
		else if (matchAny(3, EQ, ADD_EQ, SUB_EQ)) {
			prev();
			return parseAssignmentExpression(t);
		}
		else
			return parseIdentifier(t);
	} else if (matchAny(4, DEC, HEX, OCTAL, BIN)) {
		Value *l = toValue(current());
		if (matchAny(13, ADD, SUB, MUL, DIV, LAND, LOR, BAND, BOR, BXOR, REM,
				RSHIFT, LSHIFT, POW))
			return binaryExpr(l);
		printf("< expr returning literal \x1b[93m%s\x1b[0m\n",
			getTokenContent(parser.code, current()));
		return freeValue(l),
			   toValue(initLiteral(getTokenContent(parser.code, current())));
	} else if (match(STR)) {
		Value *l = toValue(current());
		printf("< expr returning string literal \x1b[92m\"%s\"\x1b[0m\n",
			getTokenContent(parser.code, current()));

		// todo add binary expression

		return freeValue(l), parseStringLiteral();
	}
	return parseLiteral();
}

Value *toValue(void *x) {
	if (!x) return NULL;
	Value *r = malloc(sizeof(Value));
	r->error = false;
	r->value = x;
	return r;
}

Value *parseImportSpecifier() {
	Value *n = fmatch(NAME, "unexpected token (9)");
	if (check(n)) return freeValue(n);
	if (!match(AS))
		return toValue(initImportSpecifier(
			initIdentifier(getTokenContent(parser.code, n->value)),
			initIdentifier(getTokenContent(parser.code, n->value))));
	Value *ln = fmatch(NAME, "expected identifier");
	if (check(ln)) return freeValue(ln);
	return toValue(initImportSpecifier(
		initIdentifier(getTokenContent(parser.code, n->value)),
		initIdentifier(getTokenContent(parser.code, ln->value))));
}

Value *impSpecArray() {
	Array *a = newArray(2);
	if (match(RCURLY)) return toValue(a);
	Value *f = parseImportSpecifier();
	if (check(f)) return freeArray(a), freeValue(f);
	pushArray(a, f->value);
	while (match(SEP)) {
		Value *s = parseImportSpecifier();
		if (check(s)) return freeArray(a), freeValue(s);
		pushArray(a, s->value);
	};
	Value *e = fmatch(RCURLY, "expected }");
	if (check(e)) return freeArray(a), freeValue(e);
	trimArray(a);
	return toValue(a);
}

Value *parseImportDeclaration() {
	Value *n = fmatch(STR, "unexpected token (1)");
	if (check(n)) return freeValue(n);
	Value *i = fmatch(IMPORT, "unexpected token (2)");
	if (check(i)) return freeValue(i), freeValue(n);
	if (match(LCURLY)) {
		Value *speca = impSpecArray();
		if (check(speca)) return freeValue(speca), freeValue(n), freeValue(i);
		match(DELMT);
		return toValue(initImportDeclaration(
			initStringLiteral(getTokenContent(parser.code, n->value)),
			speca->value));
	} else {
		Value *spec = parseImportSpecifier();
		if (check(spec)) return freeValue(spec), freeValue(n), freeValue(i);
		Array *speca = newArray(1);
		pushArray(speca, spec->value);
		match(DELMT);
		return toValue(initImportDeclaration(
			initStringLiteral(getTokenContent(parser.code, n->value)), speca));
	}
}

Value *parseExpressionStatement() {
	debug_log("exprStmt", parser.code, peekNext());
	Value *e = parseExpression();
	if (check(e)) return freeValue(e);
	// if (check(e)) {
	// 	printf("test: %p\n", e);
	// 	pushArray(parser.errors, error("Parse", parser.file, "no expression",
	// current())); 	return nop(e);
	// }
	delmt(false);

	Value *v = toValue(initExpressionStatement(e->value));

	return v;
}

Value *parseIfStatement() {
	// Value *condition = expr();
	return NULL;
} // todo

Value *parseWhileStatement() { return NULL; } // todo

Value *parseForStatement() { return NULL; } // todo

Value *parseReturnStatement() {
	debug_log("retstmt", parser.code, peekNext());
	Value *retval = parseExpression();
	if (check(retval)) return NULL;
	return toValue(initReturnStatement(retval->value));
} // todo

Value *parseStatement() {
	debug_log("stmt", parser.code, peekNext());
	if (match(FOR)) return parseForStatement();
	else if (match(WHILE))
		return parseWhileStatement();
	else if (match(RETURN))
		return parseReturnStatement();
	else if (match(IF))
		return parseIfStatement();
	return parseExpressionStatement();
}

Value *parseDeclaration() {
	debug_log("decl", parser.code, peekNext());
	if (match(FROM)) return parseImportDeclaration();
	if (match(CLASS)) return parseClassDeclaration();
	if (nextIs(FUNC) || nextIs(ASYNC)) return parseFunctionDeclaration();
	return parseStatement();
}

void synchronize() {
	debug_log("synchronize", parser.code, peekNext());
	while (!end()) {
		if (peekNext()) switch (peekNext()->type) {
				case CLASS:
				case IF:
				case FUNC:
				case FOR:
				case WHILE:
				case DO:
				case FROM:
				case RETURN:
				case ASYNC:
					return;
				case DELMT:
					next();
					return;
				default:
					next();
			};
	}
}

Result *parse(char *code, Array *tokens, char *fname) {
	printf("\n");
	initParser(code, tokens, fname);
	Result *result = malloc(sizeof(Result));
	while (!end()) {
		printf("> loop\n");
		Value *d = parseDeclaration();
		if (check(d)) synchronize();
		else {
			printf("< pushing value, type: %i\n", ((Typed *)d->value)->type);
			pushArray(parser.program->body, d->value);
		}
	}
	printf("\n");
	trimArray(parser.errors);

	result->errors = parser.errors;
	result->data   = parser.program;

	return result;
}
