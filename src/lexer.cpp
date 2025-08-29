#include "lexer.hpp"
#include "halloc.hpp"

#include <cmath>

void Lexer::reset(const Input &input) {
	this->input  = input;
	end          = false;
	had_float    = false;
	current_char = '\0';
	next_char    = this->input.next();
	iterator     = 0;
	character    = 1;
	line         = 1;
	start        = 0;
	absStart     = 0;
}

static bool isDec(const char c) { return c >= '0' && c <= '9'; }

static bool isHex(const char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	(c >= 'A' && c <= 'F');
}

static bool isAlpha(const char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' ||
	c == '_';
}

Type keyword(const char *k) {
	switch (k[0]) {
		case 'a':
			if (k[1] == 's') {
				if (k[2] == '\0')
					return Types::AS;
				if (k[2] == 'y' && k[3] == 'n' && k[4] == 'c' && k[5] == '\0')
					return Types::ASYNC;
			}
			if (k[1] == 'w' && k[2] == 'a' && k[3] == 'i' && k[4] == 't' &&
				k[5] == '\0')
				return Types::AWAIT;
			return Types::NAME;
		case 'c':
			if (k[1] == 'a' && k[2] == 's' && k[3] == 'e' && k[4] == '\0')
				return Types::CASE;
			if (k[1] == 'l' && k[2] == 'a' && k[3] == 's' && k[4] == 's' &&
				k[5] == '\0')
				return Types::CLASS;
			return Types::NAME;
		case 'd':
			if (k[1] == 'e' && k[2] == 'f' && k[3] == 'a' && k[4] == 'u' &&
				k[5] == 'l' && k[6] == 't' && k[7] == '\0')
				return Types::DEFAULT;
			if (k[1] == 'o' && k[2] == '\0')
				return Types::DO;
			return Types::NAME;
		case 'e':
			if (k[1] == 'l' && k[2] == 's' && k[3] == 'e' && k[4] == '\0')
				return Types::ELSE;
			return Types::NAME;
		case 'f':
			if (k[1] == 'a' && k[2] == 'l' && k[3] == 's' && k[4] == 'e' &&
				k[5] == '\0')
				return Types::FALSE;
			if (k[1] == 'o' && k[2] == 'r' && k[3] == '\0')
				return Types::FOR;
			if (k[1] == 'r' && k[2] == 'o' && k[3] == 'm' && k[4] == '\0')
				return Types::FROM;
			if (k[1] == 'x' && k[2] == '\0')
				return Types::FX;
			return Types::NAME;
		case 'i':
			if (k[1] == 'f' && k[2] == '\0')
				return Types::IF;
			if (k[1] == 'm' && k[2] == 'p' && k[3] == 'o' && k[4] == 'r' &&
				k[5] == 't' && k[6] == '\0')
				return Types::IMPORT;
			return Types::NAME;
		case 'n':
			if (k[1] == 'e' && k[2] == 'w' && k[3] == '\0')
				return Types::NEW;
			if (k[1] == 'u' && k[2] == 'l' && k[3] == 'l' && k[4] == '\0')
				return Types::NUL;
			return Types::NAME;
		case 'r':
			if (k[1] == 'e' && k[2] == 't' && k[3] == 'u' && k[4] == 'r' &&
				k[5] == 'n' && k[6] == '\0')
				return Types::RETURN;
			return Types::NAME;
		case 's':
			if (k[1] == 'e' && k[2] == 'l' && k[3] == 'e' && k[4] == 'c' &&
				k[5] == 't' && k[6] == '\0')
				return Types::SELECT;
			if (k[1] == 'w' && k[2] == 'i' && k[3] == 't' && k[4] == 'c' &&
				k[5] == 'h' && k[6] == '\0')
				return Types::SWITCH;
			return Types::NAME;
		case 't':
			if (k[1] == 'r' && k[2] == 'u' && k[3] == 'e' && k[4] == '\0')
				return Types::TRUE;
			return Types::NAME;
		case 'w':
			if (k[1] == 'h' && k[2] == 'i' && k[3] == 'l' && k[4] == 'e' &&
				k[5] == '\0')
				return Types::WHILE;
			return Types::NAME;
		default:
			return Types::NAME;
	}
}

char Lexer::next() {
	if (end) {
		return '\0';
	}
	current_char = next_char;
	next_char    = input.next(); // save the next char for peeking
	character++;
	iterator++;
	if (current_char == '\0') {
		end = true;
		return current_char;
	}
	if (current_char == '\n') {
		line++;
		character = 1;
	}
	return current_char;
}

char Lexer::current() const { return current_char; }

char Lexer::peek() const { return next_char; }
char Lexer::peek2() const { return input.peek(); }

bool Lexer::match(const char c) {
	if (peek() == c) {
		next();
		return true;
	}
	return false;
}

// function to create a token
Token Lexer::emit(const Type type) {
	static uint8_t constant_index = 0;
	Token token{};

	token.type         = type;
	token.pos.start    = start;
	token.pos.absStart = absStart;
	token.pos.end      = character;
	token.pos.absEnd   = iterator;
	token.pos.line     = line;

	switch (type) {
		case Types::STR: {
			const size_t len = token.pos.absEnd - token.pos.absStart;
			token.value.ptr  = halloc(len - 1);
			token.index      = constant_index++;
			input.read_chunk(
				static_cast<char *>(token.value.ptr), token.pos.absStart + 1, len - 2);
			break;
		}
		case Types::NAME: {
			const size_t len = token.pos.absEnd - token.pos.absStart;
			token.value.ptr  = halloc(len + 1);
			input.read_chunk(
				static_cast<char *>(token.value.ptr), token.pos.absStart, len);
			break;
		}
		case Types::DEC: {
#define MAX_NUMBER_LENGTH 0x40
			char buffer[MAX_NUMBER_LENGTH];
			input.read_chunk(buffer, token.pos.absStart, MAX_NUMBER_LENGTH);
			#undef MAX_NUMBER_LENGTH
			token.value.f64 = strtod(buffer, nullptr);
			token.index     = constant_index++;
			break;
		}
		case Types::OCTAL: {
#define MAX_NUMBER_LENGTH 0x40
			char buffer[MAX_NUMBER_LENGTH];
			input.read_chunk(buffer, token.pos.absStart, MAX_NUMBER_LENGTH);
			break;
		}
		default:
			had_float = false;
			break;
	}

	// Logger::print_token(token);

	return token;
}

Token Lexer::emit(const Type type, const double value) const {
	Token token;

	token.type         = type;
	token.pos.start    = start;
	token.pos.absStart = absStart;
	token.pos.end      = character;
	token.pos.absEnd   = iterator;
	token.pos.line     = line;
	token.value.f64    = value;

	// Logger::print_token(token);

	return token;
}

Token Lexer::number(const char first_char) {
	// Tested approaches:
	// . switch + bitfield
	// . switch + booleans
	// . if branching
	// This one has proved to be the fastest.

	double  value     = 0;
	double  loose     = 0;
	double  exponent  = 0;
	double  radix     = 10;
	uint8_t float_idx = 1;

	had_float      = false;
	bool done      = false;
	bool had_exp   = false;
	bool is_hex    = false;
	bool is_binary = false;
	bool is_octal  = false;
	bool oct_loose = false;
	bool set_type  = false;
	bool had_value = false;
	bool had_under = false;

	bool is_exp_neg = false;

#define insert(x)                                                              \
	if (is_binary || is_octal)                                                 \
		value = value * radix + (x);                                           \
	else if (had_exp)                                                          \
		exponent = exponent * 10 + (x);                                        \
	else if (had_float)                                                        \
		value += (x) * (1.0 / pow(radix, float_idx++));                        \
	else                                                                       \
		value = value * radix + (x);
#define insert_hex(x)                                                          \
	if (had_float)                                                             \
		value += (x) * (1.0 / pow(radix, float_idx++));                        \
	else                                                                       \
		value = value * 16 + (x);
#define insert_octal(x)                                                        \
	if (oct_loose)                                                             \
		loose = loose * 8 + (x);

	switch (first_char) {
		case '.':
			had_float = true;
			break;
		case '0':
			set_type  = true;
			oct_loose = true;
			if (isDec(peek()))
				had_under = true;
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			had_value = true;
			value     = first_char - '0';
			break;
		default:
			return emit(Types::ERROR);
	}

	for (;;) {
		switch (const char c = peek()) {
			case '0':
				had_value = true;
				insert(0);
				insert_octal(0);
				next();
				break;
			case '1':
				had_value = true;
				had_under = false;
				insert(1);
				insert_octal(1);
				next();
				break;
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				if (is_binary)
					return emit(Types::ERROR);
				had_value = true;
				had_under = false;
				insert(c - '0');
				insert_octal(c - '0');
				next();
				break;
			case '8':
			case '9':
				if (is_binary)
					return emit(Types::ERROR);
				if (is_octal)
					return emit(Types::ERROR);
				had_value = true;
				had_under = false;
				oct_loose = false;
				insert(c - '0');
				next();
				break;
			case 'a':
			case 'c':
			case 'd':
			case 'f':
				if (!is_hex || had_exp)
					return emit(Types::ERROR);
				had_value = true;
				had_under = false;
				insert_hex(c - 'a' + 10);
				next();
				break;
			case 'A':
			case 'C':
			case 'D':
			case 'F':
				if (!is_hex || had_exp)
					return emit(Types::ERROR);
				had_value = true;
				had_under = false;
				insert_hex(c - 'A' + 10);
				next();
				break;
			case 'b':
			case 'B':
				if (is_hex) {
					had_value = true;
					had_under = false;
					insert_hex(11);
					next();
					break;
				}
				if (!set_type || had_value)
					return emit(Types::ERROR);
				set_type  = false;
				oct_loose = false;
				is_binary = true;
				radix     = 2;
				had_exp   = true; // binary number cannot have exponent
				had_float = true; // binary number cannot have floating point
				next();
				break;
			case 'o':
			case 'O':
				if (!set_type || had_value)
					return emit(Types::ERROR);
				set_type  = false;
				oct_loose = false;
				is_octal  = true;
				radix     = 8;
				had_exp   = true; // octal number cannot have exponent
				had_float = true; // octal number cannot have floating point
				next();
				break;
			case 'x':
			case 'X':
				if (!set_type || had_value)
					return emit(Types::ERROR);
				set_type  = false;
				oct_loose = false;
				is_hex    = true;
				radix     = 16;
				next();
				if (!isHex(peek()) && peek() != '.')
					return emit(Types::ERROR);
				break;
			case '.':
				if (peek2() == '.') {
					done = true;
					break;
				}
				if (is_binary)
					return emit(Types::ERROR);
				if (had_float)
					return emit(Types::ERROR);
				next();
				if (!had_value && !isHex(peek()))
					return emit(Types::ERROR);
				had_float = true;
				oct_loose = false;
				break;
			case 'e':
			case 'E':
				if (is_hex) {
					had_value = true;
					insert_hex(14);
					next();
					break;
				}
				if (had_exp)
					return emit(Types::ERROR);
				next();
				if (match('+') || match('-'))
					is_exp_neg = current() == '-';
				if (!isDec(peek()))
					return emit(Types::ERROR);
				had_exp   = true;
				oct_loose = false;
				break;
			case 'p':
			case 'P':
				if (!is_hex || had_exp)
					return emit(Types::ERROR);
				had_exp = true;
				next();
				if (match('+') || match('-'))
					is_exp_neg = current() == '-';
				if (!isDec(peek()))
					return emit(Types::ERROR);
				break;
			case '_':
			case '\'':
				if (had_under)
					return emit(Types::ERROR);
				had_under = true;
				next();
				break;
			default:
				done = true;
				break;
		}
		if (done)
			break;
	}

	if (had_under)
		return emit(Types::ERROR);
	if (is_hex && had_float && !had_exp)
		return emit(Types::ERROR);

	if (is_exp_neg)
		value /= pow(is_hex ? 2.0 : 10.0, exponent);
	else
		value *= pow(is_hex ? 2.0 : 10.0, exponent);

	if (is_hex)
		return emit(Types::HEX, value);
	if (is_binary)
		return emit(Types::BINARY, value);
	if (is_octal)
		return emit(Types::OCTAL, value);
	if (oct_loose)
		return emit(Types::OCTAL, loose);
	return emit(Types::DEC, value);
}

Token Lexer::advance() {
	if (end)
		return emit(Types::END);

	for (;;) {
		const char c = next();
		start        = character - 1;
		absStart     = iterator - 1;
		switch (c) {
			case '@':
				return emit(Types::AT);
			case '.':            // .
				if (isDec(peek())) // float
					return number('.');
				if (match('.')) { // ..
					if (match('=')) // ..=
						return emit(Types::RANGE_R_IN);
					if (peek() == '.') { // ...
						if (!had_float)
							return next(), emit(Types::ERROR);
						if (peek2() == '.')
							return next(), next(), emit(Types::ERROR);
						return emit(Types::RANGE_EXCL);
					}
					return emit(Types::RANGE_EXCL);
				}
				return emit(Types::DOT);
			case ',':
				return emit(Types::COMMA);
			case '#':
				if (match('='))
					return emit(Types::CST_EQ);
				return emit(Types::HASH);
			case '$':
				if (match('='))
					return emit(Types::SET_EQ);
				return emit(Types::DOLLAR);
			case '~':
				return emit(Types::B_NOT);
			case ';':
				return emit(Types::SEMICOLON);
			case ':':
				return emit(Types::COLON);
			case '\0':
				return emit(Types::END);
			case '(':
				// todo add stack
				return emit(Types::L_PAREN);
			case ')':
				return emit(Types::R_PAREN);
			case '[':
				return emit(Types::L_BRACKET);
			case ']':
				return emit(Types::R_BRACKET);
			case '{':
				return emit(Types::L_CURLY);
			case '}':
				return emit(Types::R_CURLY);
			case '\\':
			case '\n':
			case ' ':
			case '\t':
			case '\r':
				continue;
			case '+':
				if (match('='))
					return emit(Types::ADD_EQ);
				if (match('+'))
					return emit(Types::INCR);
				return emit(Types::ADD);
			case '-':
				if (match('='))
					return emit(Types::SUB_EQ);
				if (match('-'))
					return emit(Types::DECR);
				return emit(Types::SUB);
			case '*':
				if (match('='))
					return emit(Types::MUL_EQ);
				if (match('*'))
					return emit(Types::POW);
				return emit(Types::MUL);
			case '!':
				if (match('='))
					return emit(Types::CMP_NEQ);
				return emit(Types::L_NOT);
			case '?':
				return emit(Types::QUERY);
			case '|':
				if (match('|')) {
					if (match('='))
						return emit(Types::L_OR_EQ);
					return emit(Types::L_OR);
				}
				if (match('='))
					return emit(Types::B_OR_EQ);
				return emit(Types::B_OR);
			case '&':
				if (match('&')) {
					if (match('='))
						return emit(Types::L_AND_EQ);
					return emit(Types::L_AND);
				}
				if (match('='))
					return emit(Types::B_AND_EQ);
				return emit(Types::B_AND);
			case '^':
				if (match('='))
					return emit(Types::B_XOR_EQ);
				return emit(Types::CARET);
			case '/':
				if (match('/')) {
					while (peek() != '\0' && peek() != '\n')
						next();
					continue;
				}
				if (match('*')) {
					while (peek() != '\0' && !match('*') && !match('/'))
						next();
					continue;
				}
				if (match('='))
					return emit(Types::DIV_EQ);
				return emit(Types::DIV);
			case '=':
				if (match('='))
					return emit(Types::CMP_EQ);
				if (peek() == '.' && peek2() == '.') {
					next(), next(); // consume the two periods
					return emit(match('=') ? Types::RANGE_INCL : Types::RANGE_L_IN);
				}
				return emit(Types::EQ);
			case '>':
				if (match('>')) {
					if (match('='))
						return emit(Types::R_SHIFT_EQ);
					return emit(Types::R_SHIFT);
				}
				if (match('='))
					return emit(Types::CMP_GEQ);
				return emit(Types::CMP_GT);
			case '<':
				if (match('<')) {
					if (match('='))
						return emit(Types::L_SHIFT_EQ);
					return emit(Types::L_SHIFT);
				}
				if (match('='))
					return emit(Types::CMP_LEQ);
				return emit(Types::CMP_LT);
			case '%':
				if (match('='))
					return emit(Types::REM_EQ);
				return emit(Types::REM);
			case '"': {
				bool err = false;
				while (peek() != '"') {
					if (peek() == '\n' || peek() == '\0') {
						err = true;
						break;
					}
					if (peek() == '\\')
						next();
					next();
				}
				if (err) {
					Logger::error("Unterminated string");
					continue;
				}
				next();
				return emit(Types::STR);
			}
			case '`': {
				bool err = false;
				// multiline strings
				while (peek() != '`') {
					if (peek() == '\0') {
						err = true;
						break;
					}
					if (peek() == '\\')
						next();
					next();
				}
				if (err) {
					Logger::error("Unterminated string");
					continue;
				}
				next();
				return emit(Types::STR);
			}
			case '\'': {
				bool err = false;
				while (peek() != '\'') {
					if (peek() == '\n' || peek() == '\0') {
						err = true;
						break;
					}
					if (peek() == '\\')
						next();
					next();
				}
				if (err) {
					Logger::error("Unterminated string");
					continue;
				}
				next();
				return emit(Types::STR);
			}

			default: {
				if (isDec(current()))
					return number(current());

				if (isAlpha(current())) {
					// matching keywords and names

					while (isAlpha(peek()) || isDec(peek()))
						next();
					const int len = iterator + 1 - absStart;
					char      buffer[0xFF]; // max length of 255
					input.read_chunk(buffer, absStart, len);
					buffer[len] = '\0';

					return emit(keyword(buffer));
				}

				// Parse UTF8 characters
				if ((current() & 0xe0) == 0xc0)
					next();
				else if ((current() & 0xf0) == 0xe0)
					next(), next();
				else if ((current() & 0xf8) == 0xf0)
					next(), next(), next();
				Token t = emit(Type::NUL);
				Logger::error("Unexpected token", &t, input.get_name());
			}
		}
	}
}
