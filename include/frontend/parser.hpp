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

		std::vector<Token> parse_qualified_name();
		ModuleDecl parse_module();
		ImportDecl parse_import();
		Stmt top_level_declaration();

		Type parse_type();

		Expr expression();
		Expr equality();
		Expr comparison();
		Expr term();
		Expr factor();
		Expr unary();
		Expr primary();

		Stmt declaration();
		Stmt var_declaration();
		std::vector<Param> parse_params();
		Stmt function_declaration();
		Stmt statement();
		Stmt expression_statement();
		Stmt if_statement();
		Stmt while_statement();
		Stmt return_statement();
		Stmt struct_declaration();
		Stmt enum_declaration();
		std::vector<Stmt> block();
		Stmt parse_block_stmt();

		void synchronize();

	public:
		explicit Parser(std::vector<Token> tokens);

		[[nodiscard]] CompilationUnit parse();
	};
} // namespace hadron::frontend
