#pragma once

#include <string_view>
#include <vector>

#include "token.hpp"

namespace hadron::frontend {
	class Lexer {
		std::vector<std::string> string_pool;
		std::string_view source_;
		usize cursor_ = 0;
		usize start_ = 0;
		u32 line_ = 1;
		u32 col_ = 1;

		[[nodiscard]] char peek(usize offset = 0) const;
		char advance();
		bool match(char expected);

		Token make_token(TokenType type, usize length) const;
		Token make_string_token(std::string text, const usize length);
		Token make_error_token(std::string_view message) const;
		void skip_whitespace();
		Token parse_identifier();
		Token parse_number();
		Token parse_string();

	public:
		explicit Lexer(std::string_view source);

		Lexer(const Lexer &) = delete;

		Lexer &operator=(const Lexer &) = delete;

		Lexer(Lexer &&) = default;

		Lexer &operator=(Lexer &&) = default;

		[[nodiscard]] std::vector<Token> tokenize();
	};
} // namespace hadron::frontend
