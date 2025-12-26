#pragma once

#include <optional>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"

namespace hadron::frontend {
	class Parser {
		std::vector<Token> tokens_;
		usize current_ = 0;

		[[nodiscard]] const Token &peek() const;
		[[nodiscard]] const Token &previous() const;
		[[nodiscard]] bool is_at_end() const;
		[[nodiscard]] bool check(TokenType type) const;
		Token advance();
		bool match(const std::vector<TokenType> &types);
		Token consume(TokenType type, std::string_view message);

		Expr expression();
		Expr equality();
		Expr comparison();
		Expr term();
		Expr factor();
		Expr unary();
		Expr primary();

		Stmt declaration();
		Stmt var_declaration();
		Stmt statement();
		Stmt expression_statement();
		std::vector<Stmt> block();

		void synchronize();

	public:
		explicit Parser(std::vector<Token> tokens);

		[[nodiscard]] std::vector<Stmt> parse();
	};
} // namespace hadron::frontend
