#include "tokenizer.h"

static void initTokenizer(string);
static char next(void);
static char peekNext(void);
// static char peekPrev(void);
static char current(void);
static Token *addToken(Type);
static bool match(char);
static bool end(void);
static bool start(void);
static bool isAlpha(char);
static bool isDec(char);
static bool isBin(char);
static bool isOct(char);
static bool isHex(char);

struct Tokenizer {
	Position token;
	int line;
	int iterator;
	int character;
	int length;
	Array *tokens;
	Array *errors;
	string code;
} tokenizer;

void initTokenizer(string code) {
	if (tokenizer.tokens) freeArray(tokenizer.tokens);
	if (tokenizer.errors) freeArray(tokenizer.errors);
	tokenizer.code = code;
	tokenizer.length = strlen(code);
	tokenizer.iterator = -1;
	tokenizer.token = (Position){ { 1, 1 }, { 1, 1 }, 1, 1 };
	tokenizer.line = 1;
	tokenizer.character = 1;
	tokenizer.tokens = newArray(tokenizer.length/8);
	tokenizer.errors = newArray(2);
}

Result *tokenize(string code) {
	initTokenizer(code);
	char c;
	while (!end()) {
		c = next();
		tokenizer.token.start = (Point){ tokenizer.line, tokenizer.character-1 };
		tokenizer.token.absoluteStart = tokenizer.iterator;
		switch (c) {
			case '@': addToken(AT); continue;
			case '.': addToken(DOT); continue;
			case ',': addToken(SEP); continue;
			case ';': addToken(DELMT); continue;
			case '{': case '}': addToken(CBRACKET); continue;
			case '[': case ']': addToken(BRACKET); continue;
			case '(': case ')': addToken(PAREN); continue;
			case '\0': addToken(_EOF); continue;
			case '+': {
				if (match('=')) addToken(ADD_EQ);
				else if (match('+')) addToken(INCR);
				else addToken(ADD);
				continue;
			} case '-': {
				if (match('=')) addToken(SUB_EQ);
				else if (match('-')) addToken(DECR);
				else addToken(SUB);
				continue;
			} case '*': {
				if (match('=')) addToken(MUL_EQ);
				else if (match('=')) addToken(POW);
				else addToken(MUL);
				continue;
			} case '|': {
				if (match('|')) {
					if (match('=')) addToken(LOR_EQ);
					else addToken(LOR);
				} else if (match('=')) addToken(BOR_EQ);
				else addToken(BOR);
				continue;
			} case '&': {
				if (match('&')) {
					if (match('=')) addToken(LAND_EQ);
					else addToken(LAND);
				} else if (match('=')) addToken(BAND_EQ);
				else addToken(BAND);
				continue;
			} case '~': {
				if (match('=')) addToken(BNOT_EQ);
				else addToken(BNOT);
				continue;
			} case '^': {
				if (match('=')) addToken(BXOR_EQ);
				else addToken(BXOR);
				continue;
			} case '/': {
				if (match('/')) while (peekNext() != '\n' && !end()) next();
				else if (match('*')) { while (!(next() == '*' && next() == '/') && !end()); }
				else addToken(DIV);
				continue;
			} case '=': {
				if (match('=')) addToken(CMP_EQ);
				else addToken(EQ);
				continue;
			} case '>': {
				if (match('>')) {
					if (match('=')) addToken(RSHIFT_EQ);
					else addToken(RSHIFT);
				} else if (match('=')) addToken(CMP_GEQ);
				else addToken(CMP_GT);
				continue;
			} case '<': {
				if (match('>')) {
					if (match('=')) addToken(LSHIFT_EQ);
					else addToken(LSHIFT);
				} else if (match('=')) addToken(CMP_LEQ);
				else addToken(CMP_LT);
				continue;
			} case '%': {
				if (match('=')) addToken(REM_EQ);
				else addToken(REM);
				continue;
			} case '"': {
				bool err = false;
				while (peekNext() != '"') {
					if (next() == '\n') { err = true; break; }
				}; next();
				Token *token = addToken(STR);
				if (err) pushArray(tokenizer.errors, error(
					"Syntax", "temp/index.idk",
					"unterminated string", token
				)); continue;
			} default: {
				if (match('0')) {
					char base = next();
					if (base == 'o') {
						while (isOct(peekNext())) { next(); }
					} else if (base == 'b') {
						while (isBin(peekNext())) { next(); }
					} else if (base == 'x') {
						while (isHex(peekNext())) { next(); }
					}
				}
				if (isDec(current())) {
					while (isDec(peekNext())) next();
					addToken(DEC);
				}
				if (isAlpha(current())) {
					while (isAlpha(peekNext()) || isDec(peekNext())) next();
					int len = tokenizer.iterator+1 - tokenizer.token.absoluteStart;
					string substr = malloc(len+1);
					memcpy(substr, &code[tokenizer.token.absoluteStart], len);
					substr[len] = '\0';
					if (!strcmp(substr, "for")) { addToken(FOR); free(substr); continue; }
					if (!strcmp(substr, "class")) { addToken(CLASS); free(substr); continue; }
					if (!strcmp(substr, "func")) { addToken(FUNC); free(substr); continue; }
					if (!strcmp(substr, "true")) { addToken(TRUE); free(substr); continue; }
					if (!strcmp(substr, "false")) { addToken(FALSE); free(substr); continue; }
					if (!strcmp(substr, "null")) { addToken(_NULL); free(substr); continue; }
					if (!strcmp(substr, "while")) { addToken(WHILE); free(substr); continue; }
					if (!strcmp(substr, "if")) { addToken(IF); free(substr); continue; }
					if (!strcmp(substr, "do")) { addToken(DO); free(substr); continue; }
					if (!strcmp(substr, "else")) { addToken(ELSE); free(substr); continue; }
					if (!strcmp(substr, "from")) { addToken(FROM); free(substr); continue; }
					if (!strcmp(substr, "import")) { addToken(IMPORT); free(substr); continue; }
					if (!strcmp(substr, "new")) { addToken(NEW); free(substr); continue; }
					if (!strcmp(substr, "await")) { addToken(AWAIT); free(substr); continue; }
					if (!strcmp(substr, "as")) { addToken(AS); free(substr); continue; }
					if (!strcmp(substr, "async")) { addToken(ASYNC); free(substr); continue; }
					if (!strcmp(substr, "ret")) { addToken(RET); free(substr); continue; }
					addToken(NAME); free(substr);
				}; continue;
			};
		}
	}
	Result *result = malloc(sizeof(Result));
	trimArray(tokenizer.tokens);
	result->errors = tokenizer.errors;
	result->data = tokenizer.tokens;
	return result;
}

Token *addToken(Type type) {
	Token *token = malloc(sizeof(Token));
	token->type = type;
	token->pos.absoluteStart = tokenizer.token.absoluteStart;
	token->pos.absoluteEnd = tokenizer.iterator+1;
	token->pos.start = tokenizer.token.start;
	token->pos.end = (Point){ tokenizer.line, tokenizer.character };
	pushArray(tokenizer.tokens, token);
	return token;
};
char peekNext() {
	if (!end()) return tokenizer.code[tokenizer.iterator+1];
	else return '\0';
}
char peekPrev() {
	if (!start()) return tokenizer.code[tokenizer.iterator-1];
	else return '\0';
}
char next() {
	char r = !end() ? tokenizer.code[++tokenizer.iterator] : '\0';
	tokenizer.character++;
	if (r == '\n') {
		tokenizer.line++;
		tokenizer.character = 1;
	}; return r;
}
char current() { return tokenizer.code[tokenizer.iterator]; }
bool end() { return tokenizer.iterator > tokenizer.length-1; }
bool start() { return tokenizer.iterator < 1; }
bool match(char c) {
	if (peekNext() == c) { next(); return true; }
	else return false;
}
bool isDec(char c) { return c >= '0' && c <= '9'; }
bool isBin(char c) { return c == '0' ||  c == '1'; }
bool isOct(char c) { return c >= '0' && c <= '7'; }
bool isHex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_'; }
