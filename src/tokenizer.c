#include "tokenizer.h"

static void initTokenizer(char *, char *);
static void runDistanceCheck(Array *);
static void runStackCheck(void);
static char next(void);
static void consume(int);
static char peekNext(void);
// static char peekPrev(void);
static char	   current(void);
static Token  *addToken(Type);
static Token  *cloneToken(Token *);
static boolean match(char);
static boolean end(void);
static boolean start(void);
static boolean isAlpha(char);
static boolean isDec(char);
static boolean isBin(char);
static boolean isOct(char);
static boolean isHex(char);

static struct Tokenizer {
	Position token;
	int		 line;
	int		 iterator;
	int		 character;
	int		 length;
	Array	*tokens;
	Array	*errors;
	Array	*stack;
	char	*code;
	char	*file;
} tokenizer;

void initTokenizer(char *code, char *fname) {
	if (tokenizer.tokens) freeArray(tokenizer.tokens);
	if (tokenizer.errors) freeArray(tokenizer.errors);
	tokenizer.code		= code;
	tokenizer.length	= strlen(code);
	tokenizer.iterator	= -1;
	tokenizer.token		= (Position){1, 1, 1, 1, 1};
	tokenizer.line		= 1;
	tokenizer.character = 1;
	tokenizer.tokens	= newArray(tokenizer.length / 8);
	tokenizer.errors	= newArray(2);
	tokenizer.stack		= newArray(4);
	tokenizer.file		= fname;
}

Token *cloneToken(Token *t) {
	Token *n = malloc(sizeof(Token));
	n->pos	 = t->pos;
	n->type	 = t->type;
	return n;
}

Result *tokenize(char *code, char *fname) {
	initTokenizer(code, fname);
	// syncStack();
	char c;
	while (!end()) {
		c						 = next();
		tokenizer.token.start	 = tokenizer.character - 1;
		tokenizer.token.absStart = tokenizer.iterator;
		switch (c) {
			case '@':
				addToken(AT);
				continue;
			case '.':
				addToken(DOT);
				continue;
			case ',':
				addToken(SEP);
				continue;
			case '#':
				addToken(HASH);
				continue;
			case '~':
				addToken(BNOT);
				continue;
			case ';':
				addToken(DELMT);
				continue;
			case ':':
				addToken(COLON);
				continue;
			case '\0':
				addToken(_EOF);
				continue;
			case '(':
				pushArray(tokenizer.stack, cloneToken(addToken(LPAREN)));
				continue;
			case ')':
				pushArray(tokenizer.stack, cloneToken(addToken(RPAREN)));
				continue;
			case '[':
				pushArray(tokenizer.stack, cloneToken(addToken(LBRACKET)));
				continue;
			case ']':
				pushArray(tokenizer.stack, cloneToken(addToken(RBRACKET)));
				continue;
			case '{':
				pushArray(tokenizer.stack, cloneToken(addToken(LCURLY)));
				continue;
			case '}':
				pushArray(tokenizer.stack, cloneToken(addToken(RCURLY)));
				continue;
			case '\\':
				continue;
			case '\n':
				continue;
			case ' ':
			case '\t':
				continue;
			case '+':
				if (match('=')) addToken(ADD_EQ);
				else if (match('+'))
					addToken(INCR);
				else
					addToken(ADD);
				continue;
			case '-':
				if (match('=')) {
					addToken(SUB_EQ);
				} else if (match('-')) {
					addToken(DECR);
				} else {
					addToken(SUB);
				}
				continue;
			case '*':
				if (match('=')) addToken(MUL_EQ);
				else if (match('='))
					addToken(POW);
				else
					addToken(MUL);
				continue;
			case '!':
				if (match('=')) addToken(CMP_NEQ);
				else
					addToken(LNOT);
				continue;
			case '?':
				addToken(QUERY);
				continue;
			case '|':
				if (match('|')) {
					if (match('=')) {
						addToken(LOR_EQ);
					} else {
						addToken(LOR);
					}
				} else if (match('=')) {
					addToken(BOR_EQ);
				} else {
					addToken(BOR);
				}
				continue;
			case '&':
				if (match('&')) {
					if (match('=')) addToken(LAND_EQ);
					else
						addToken(LAND);
				} else if (match('='))
					addToken(BAND_EQ);
				else
					addToken(BAND);
				continue;
			case '^':
				if (match('=')) addToken(BXOR_EQ);
				else
					addToken(BXOR);
				continue;
			case '/':
				if (match('/'))
					while (peekNext() != '\n' && !end())
						next();
				else if (match('*')) {
					while (!(next() == '*' && next() == '/') && !end())
						;
				} else
					addToken(DIV);
				continue;
			case '=':
				if (match('=')) addToken(CMP_EQ);
				else
					addToken(EQ);
				continue;
			case '>':
				if (match('>')) {
					if (match('=')) addToken(RSHIFT_EQ);
					else
						addToken(RSHIFT);
				} else if (match('='))
					addToken(CMP_GEQ);
				else
					addToken(CMP_GT);
				continue;
			case '<':
				if (match('>')) {
					if (match('=')) addToken(LSHIFT_EQ);
					else
						addToken(LSHIFT);
				} else if (match('='))
					addToken(CMP_LEQ);
				else
					addToken(CMP_LT);
				continue;
			case '%':
				if (match('=')) addToken(REM_EQ);
				else
					addToken(REM);
				continue;
			case '"': {
				boolean err = false;
				while (peekNext() != '"') {
					if (peekNext() == '\n') {
						err = true;
						break;
					}
					if (peekNext() == '\\') next();
					next();
				};
				if (err)
					pushArray(tokenizer.errors,
						error("Syntax", fname, "unterminated string",
							addToken(STR)));
				else {
					next();
					addToken(STR);
				}
				continue;
			}
			case '\'': {
				boolean err = false;
				while (peekNext() != '\'') {
					if (peekNext() == '\n') {
						err = true;
						break;
					}
					if (peekNext() == '\\') next();
					next();
				}
				if (err)
					pushArray(tokenizer.errors,
						error("Syntax", fname, "unterminated string",
							addToken(STR)));
				else {
					next();
					addToken(STR);
				}
				continue;
			}
			default: {
				if (current() == '0') {
					if (match('o') || isOct(peekNext())) {
						while (isOct(peekNext()))
							next();
						addToken(OCTAL);
					} else if (match('b')) {
						while (isBin(peekNext()))
							next();
						addToken(BIN);
					} else if (match('x')) {
						while (isHex(peekNext()))
							next();
						addToken(HEX);
					} else
						addToken(DEC);
					continue;
				}
				if (isDec(current())) {
					while (isDec(peekNext()))
						next();
					addToken(DEC);
					continue;
				}
				if (isAlpha(current())) {
					while (isAlpha(peekNext()) || isDec(peekNext()))
						next();
					int len = tokenizer.iterator + 1 - tokenizer.token.absStart;
					char *sub = substr(code, tokenizer.token.absStart, len);
					if (!strcmp(sub, "for")) addToken(FOR);
					else if (!strcmp(sub, "class"))
						addToken(CLASS);
					else if (!strcmp(sub, "func"))
						addToken(FUNC);
					else if (!strcmp(sub, "true"))
						addToken(TRUE);
					else if (!strcmp(sub, "false"))
						addToken(FALSE);
					else if (!strcmp(sub, "null"))
						addToken(_NULL);
					else if (!strcmp(sub, "while"))
						addToken(WHILE);
					else if (!strcmp(sub, "if"))
						addToken(IF);
					else if (!strcmp(sub, "do"))
						addToken(DO);
					else if (!strcmp(sub, "else"))
						addToken(ELSE);
					else if (!strcmp(sub, "from"))
						addToken(FROM);
					else if (!strcmp(sub, "import"))
						addToken(IMPORT);
					else if (!strcmp(sub, "new"))
						addToken(NEW);
					else if (!strcmp(sub, "await"))
						addToken(AWAIT);
					else if (!strcmp(sub, "as"))
						addToken(AS);
					else if (!strcmp(sub, "async"))
						addToken(ASYNC);
					else if (!strcmp(sub, "return"))
						addToken(RETURN);
					else
						addToken(NAME);
					free(sub);
					continue;
				}

				if ((current() & 0xe0) == 0xc0) next();
				else if ((current() & 0xf0) == 0xe0)
					consume(2);
				else if ((current() & 0xf8) == 0xf0)
					consume(3);
				Token *t = addToken(UNDEF);
				pushArray(tokenizer.errors,
					error("Syntax", fname, "unexpected token", t));
				continue;
			}
		}
	}
	runStackCheck();
	trimArray(tokenizer.tokens);
	Result *result = malloc(sizeof(Result));
	result->errors = tokenizer.errors;
	result->data   = tokenizer.tokens;
	return result;
}

void runDistanceCheck(Array *r) {
	for (int dist = 1; dist < r->l - 1; dist++) {
		for (int i = 0; i + dist < r->l - 1; i++) {
			Token *a = getArray(r, i);
			Token *b = getArray(r, i + dist + 1);
			if (!a || !b) continue;
			if ((a->type == LPAREN && b->type == RPAREN) ||
				(a->type == LBRACKET && b->type == RBRACKET) ||
				(a->type == LCURLY && b->type == RCURLY)) {
				removeArray(r, i);
				removeArray(r, i + dist + 1);
			}
		}
	}
	Array *temp = clearArray(r);
	for (int i = 0; i < temp->l; i++) {
		Token *t = temp->a[i];
		pushArray(tokenizer.errors,
			error("Syntax", tokenizer.file, "unmatched token", cloneToken(t)));
	}
	freeArray(temp);
}

void runStackCheck() {
	Array *s = tokenizer.stack;
	Array *r = newArray(s->l / 2);
	for (int i = 0; i < s->l; i++) {
		Token *t = s->a[i];
		Token *p = lastArray(r);
		if (t->type == LPAREN) pushArray(r, cloneToken(t));
		else if (t->type == LBRACKET)
			pushArray(r, cloneToken(t));
		else if (t->type == LCURLY)
			pushArray(r, cloneToken(t));
		else if (p && p->type == LPAREN && t->type == RPAREN)
			popArray(r);
		else if (p && p->type == LBRACKET && t->type == RBRACKET)
			popArray(r);
		else if (p && p->type == LCURLY && t->type == RCURLY)
			popArray(r);
		else
			pushArray(r, cloneToken(t));
	}
	freeArray(s);
	runDistanceCheck(r);
}

Token *addToken(Type type) {
	Token *token		= malloc(sizeof(Token));
	token->type			= type;
	token->pos.absStart = tokenizer.token.absStart;
	token->pos.absEnd	= tokenizer.iterator + 1;
	token->pos.start	= tokenizer.token.start;
	token->pos.end		= tokenizer.character;
	token->pos.line		= tokenizer.line;
	pushArray(tokenizer.tokens, token);
	return token;
}

char peekNext() { return end() ? 0 : tokenizer.code[tokenizer.iterator + 1]; }

char peekPrev() { return start() ? 0 : tokenizer.code[tokenizer.iterator - 1]; }

char next() {
	char r = !end() ? tokenizer.code[++tokenizer.iterator] : 0;
	tokenizer.character++;
	if (r == '\n') {
		tokenizer.line++;
		tokenizer.character = 1;
	}
	return r;
}

void consume(int n) {
	while (n--)
		next();
}

char current() { return tokenizer.code[tokenizer.iterator]; }

boolean end() { return tokenizer.iterator > tokenizer.length - 1; }

boolean start() { return tokenizer.iterator < 1; }

boolean match(char c) {
	if (peekNext() == c) {
		next();
		return true;
	} else
		return false;
}

boolean isDec(char c) { return (c >= '0' && c <= '9'); }
boolean isBin(char c) { return c == '0' || c == '1'; }
boolean isOct(char c) { return c >= '0' && c <= '7'; }
boolean isHex(char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
		   (c >= 'A' && c <= 'F');
}
boolean isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' ||
		   c == '_';
}
