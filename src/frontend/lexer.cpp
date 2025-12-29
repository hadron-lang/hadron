#include "frontend/lexer.hpp"

#include <cstring>
#include <format>
#include <unordered_map>

namespace hadron::frontend {
	static const std::unordered_map<std::string_view, TokenType> KEYWORDS = {
		{"value", TokenType::KwValue},
		{"struct", TokenType::KwStruct},
		{"class", TokenType::KwClass},
		{"interface", TokenType::KwInterface},
		{"enum", TokenType::KwEnum},
		{"union", TokenType::KwUnion},
		{"if", TokenType::KwIf},
		{"else", TokenType::KwElse},
		{"for", TokenType::KwFor},
		{"while", TokenType::KwWhile},
		{"loop", TokenType::KwLoop},
		{"break", TokenType::KwBreak},
		{"continue", TokenType::KwContinue},
		{"return", TokenType::KwReturn},
		{"fx", TokenType::KwFx},
		{"static", TokenType::KwStatic},
		{"spawn", TokenType::KwSpawn},
		{"thread", TokenType::KwThread},
		{"async", TokenType::KwAsync},
		{"await", TokenType::KwAwait},
		{"var", TokenType::KwVar},
		{"val", TokenType::KwVal},
		{"null", TokenType::KwNull},
		{"true", TokenType::KwTrue},
		{"false", TokenType::KwFalse},
		{"module", TokenType::KwModule},
		{"import", TokenType::KwImport},
		{"as", TokenType::KwAs},
		{"public", TokenType::KwPub},
		{"private", TokenType::KwPriv},
		{"try", TokenType::KwTry},
		{"catch", TokenType::KwCatch},
		{"throw", TokenType::KwThrow},
		{"extern", TokenType::KwExtern},
		{"sizeof", TokenType::KwSizeOf}
	};

	std::string Token::to_string() const {
		const std::string s_type = std::string(token_type_to_string(type));
		const std::string s_text = std::string(text);

		return "Token(" + s_type + " '" + s_text + "' at " + std::to_string(location.line) + ":" +
			   std::to_string(location.column) + ")";
	}

	std::string_view token_type_to_string(const TokenType type) {
		switch (type) {
		case TokenType::Identifier:
			return "Identifier";
		case TokenType::Number:
			return "Number";
		case TokenType::String:
			return "String";
		case TokenType::KwFx:
			return "fx";
		case TokenType::Eof:
			return "EOF";
		case TokenType::Error:
			return "Error";
		default:
			return "Token";
		}
	}

	Lexer::Lexer(const std::string_view source) : source_(source) {}

	char Lexer::peek(const usize offset) const {
		if (cursor_ + offset >= source_.length())
			return '\0';
		return source_[cursor_ + offset];
	}

	char Lexer::advance() {
		const char c = source_[cursor_++];
		if (c == '\n') {
			line_++;
			col_ = 1;
		} else {
			col_++;
		}
		return c;
	}

	bool Lexer::match(const char expected) {
		if (peek() == expected) {
			advance();
			return true;
		}
		return false;
	}

	Token Lexer::make_token(const TokenType type, const usize length) const {
		const std::string_view text = source_.substr(start_, length);
		return Token{text, {line_, col_ - static_cast<u32>(length), start_}, type, {}};
	}

	Token Lexer::make_string_token(std::string value, const usize length) {
		string_pool.push_back(std::move(value));
		std::string &stored = string_pool.back();

		return Token{std::string_view(stored), {line_, col_ - static_cast<u32>(length), start_}, TokenType::String, {}};
	}

	Token Lexer::make_error_token(const std::string_view message) const {
		return Token{message, {line_, col_, start_}, TokenType::Error, {}};
	}

	void Lexer::skip_whitespace() {
		while (true) {
			switch (peek()) {
			case ' ':
			case '\r':
			case '\t':
			case '\n':
				advance();
				break;
			case '/':
				if (peek(1) == '/') {
					advance();
					advance();
					while (peek() != '\n' && peek() != '\0') {
						advance();
					}
				} else
					return;
				break;
			default:
				return;
			}
		}
	}

	Token Lexer::parse_identifier() {
		while (std::isalnum(peek()) || peek() == '_')
			advance();

		const usize length = cursor_ - start_;
		const std::string_view text = source_.substr(start_, length);
		TokenType type = TokenType::Identifier;

		if (const auto it = KEYWORDS.find(text); it != KEYWORDS.end())
			type = it->second;

		return make_token(type, length);
	}

	Token Lexer::parse_number() {
		while (std::isdigit(peek()))
			advance();

		if (peek() == '.' && std::isdigit(peek(1))) {
			advance();
			while (std::isdigit(peek()))
				advance();
		}

		return make_token(TokenType::Number, cursor_ - start_);
	}

	Token Lexer::parse_string() {
		std::string value;
		value.reserve(16);

		while (true) {
			char c = peek();

			if (c == '\0')
				return make_error_token("Unterminated string literal");

			if (c == '"') {
				advance();
				break;
			}

			if (c == '\\') {
				advance();
				char e = peek();

				if (e == '\0')
					return make_error_token("Unterminated escape sequence");

				switch (e) {
				case 'n':
					value.push_back('\n');
					break;
				case 't':
					value.push_back('\t');
					break;
				case 'r':
					value.push_back('\r');
					break;
				case '"':
					value.push_back('"');
					break;
				case '\\':
					value.push_back('\\');
					break;
				case '0':
					value.push_back('\0');
					break;
				default:
					return make_error_token("Invalid escape sequence");
				}

				advance();
				continue;
			}

			value.push_back(c);
			advance();
		}

		return make_string_token(std::move(value), cursor_ - start_);
	}

	std::vector<Token> Lexer::tokenize() {
		std::vector<Token> tokens;
		tokens.reserve(source_.length() / 4);

		while (cursor_ < source_.length()) {
			skip_whitespace();
			if (cursor_ >= source_.length())
				break;

			start_ = cursor_;

			if (const char c = advance(); std::isalpha(c) || c == '_')
				tokens.push_back(parse_identifier());
			else if (std::isdigit(c))
				tokens.push_back(parse_number());
			else if (c == '"')
				tokens.push_back(parse_string());
			else {
				TokenType type;
				usize len = 1;

				switch (c) {
				case '(':
					type = TokenType::LParen;
					break;
				case ')':
					type = TokenType::RParen;
					break;
				case '{':
					type = TokenType::LBrace;
					break;
				case '}':
					type = TokenType::RBrace;
					break;
				case '[':
					type = TokenType::LBracket;
					break;
				case ']':
					type = TokenType::RBracket;
					break;
				case ',':
					type = TokenType::Comma;
					break;
				case '.':
					if (match('.') && match('.')) {
						type = TokenType::Ellipsis;
						len = 3;
					} else
						type = TokenType::Dot;
					break;
				case ';':
					type = TokenType::Semicolon;
					break;
				case ':':
					type = TokenType::Colon;
					break;
				case '*':
					if (match('=')) {
						type = TokenType::StarEq;
						len = 2;
					} else
						type = TokenType::Star;
					break;
				case '%':
					type = TokenType::Percent;
					break;
				case '^':
					type = TokenType::Caret;
					break;
				case '~':
					type = TokenType::Bang;
					break;
				case '+':
					if (match('=')) {
						type = TokenType::PlusEq;
						len = 2;
					} else
						type = TokenType::Plus;
					break;
				case '-':
					if (match('>')) {
						type = TokenType::Arrow;
						len = 2;
					} else if (match('=')) {
						type = TokenType::MinusEq;
						len = 2;
					} else
						type = TokenType::Minus;
					break;
				case '/':
					if (match('=')) {
						type = TokenType::SlashEq;
						len = 2;
					} else
						type = TokenType::Slash;
					break;
				case '=':
					if (match('=')) {
						type = TokenType::EqEq;
						len = 2;
					} else if (match('>')) {
						type = TokenType::FatArrow;
						len = 2;
					} else
						type = TokenType::Eq;
					break;
				case '!':
					if (match('=')) {
						type = TokenType::BangEq;
						len = 2;
					} else
						type = TokenType::Bang;
					break;
				case '<':
					if (match('=')) {
						type = TokenType::LtEq;
						len = 2;
					} else
						type = TokenType::Lt;
					break;
				case '>':
					if (match('=')) {
						type = TokenType::GtEq;
						len = 2;
					} else
						type = TokenType::Gt;
					break;
				case '&':
					if (match('&')) {
						type = TokenType::And;
						len = 2;
					} else
						type = TokenType::Ampersand;
					break;
				case '|':
					if (match('|')) {
						type = TokenType::Or;
						len = 2;
					} else
						type = TokenType::Pipe;
					break;
				default:
					tokens.push_back(make_error_token("Unexpected character"));
					continue;
				}

				tokens.push_back(make_token(type, len));
			}
		}

		tokens.push_back(Token{"", {line_, col_, cursor_}, TokenType::Eof, {}});
		return tokens;
	}
} // namespace hadron::frontend
