#include "tokenizer.h"

Tokenizer tokenizer;

void initTokenizer(string code) {
	tokenizer.code = code;
	tokenizer.len = strlen(code);
	tokenizer.iterator = -1;
	tokenizer.tokenStart = 0;
	tokenizer.line = 1;
	TArray tokens;
	initTArray(&tokens);
	tokenizer.tokens = tokens;
}

TArray *tokenize(string code) {
	initTokenizer(code);
	char c;
	while (!end()) {
		c = next();
		tokenizer.tokenStart = tokenizer.iterator;
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
				else if (match('*')) while (!(next() == '*' && next() == '/') && !end());
				else addToken(DIV); continue;
			} case '=': {
				if (match('=')) addToken(CMP_EQ);
				else addToken(EQ); continue;
			} case '>': {
				if (match('>')) {
					if (match('=')) addToken(RSHIFT_EQ);
					else addToken(RSHIFT);
				} else if (match('=')) addToken(CMP_GEQ);
				else addToken(CMP_GT); continue;
			} case '<': {
				if (match('>')) {
					if (match('=')) addToken(LSHIFT_EQ);
					else addToken(LSHIFT);
				} else if (match('=')) addToken(CMP_LEQ);
				else addToken(CMP_LT); continue;
			} case '%': {
				if (match('=')) addToken(REM_EQ);
				else addToken(REM);
			} case '"': {
				while (peekNext() != '"') {
					next();
				}; next(); addToken(STR); continue;
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
					int len = tokenizer.iterator+1 - tokenizer.tokenStart;
					string substr = malloc(len+1);
					memcpy(substr, &code[tokenizer.tokenStart], len);
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
					addToken(NAME); free(substr);
				}; continue;
			};
		}
	}
	return &tokenizer.tokens;
}

void addToken(Type type) {
	Token token;
	token.type = type;
	token.line = tokenizer.line;
	token.start = tokenizer.tokenStart;
	token.end = tokenizer.iterator+1;
	pushToken(&(tokenizer.tokens), token);
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
	if (!end()) return tokenizer.code[++tokenizer.iterator];
	else return '\0';
}
char current() { return tokenizer.code[tokenizer.iterator]; }
bool end() { return tokenizer.iterator > tokenizer.len-1; }
bool start() { return tokenizer.iterator < 1; }
bool match(char c) {
	if (peekNext() == c) { next(); return true; }
	else return false;
}
bool isDec(char c) { return c >= '0' && c <= '9'; }
bool isBin(char c) { return c == '0' ||  c == '1'; }
bool isOct(char c) { return c >= '0' && c <= '7';}
bool isHex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
bool isAlpha(char c) { return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '$'; }
