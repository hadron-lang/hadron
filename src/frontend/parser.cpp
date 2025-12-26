#include <iostream>
#include <stdexcept>

#include "frontend/parser.hpp"

namespace hadron::frontend {
	Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

	std::vector<Stmt> Parser::parse() {
		std::vector<Stmt> statements;
		while (!is_at_end()) {
			try {
				statements.push_back(declaration());
			} catch (const std::runtime_error &error) {
				std::cerr << "Parse Error: " << error.what() << "\n";
				synchronize();
			}
		}
		return statements;
	}

	const Token &Parser::peek() const {
		return tokens_[current_];
	}

	const Token &Parser::previous() const {
		return tokens_[current_ - 1];
	}

	bool Parser::is_at_end() const {
		return peek().type == TokenType::Eof;
	}

	bool Parser::check(const TokenType type) const {
		if (is_at_end())
			return false;
		return peek().type == type;
	}

	Token Parser::advance() {
		if (!is_at_end())
			current_++;
		return previous();
	}

	bool Parser::match(const std::vector<TokenType> &types) {
		for (const TokenType type : types) {
			if (check(type)) {
				advance();
				return true;
			}
		}
		return false;
	}

	Token Parser::consume(const TokenType type, const std::string_view message) {
		if (check(type))
			return advance();
		throw std::runtime_error(std::string(message) + " at " + peek().to_string());
	}

	Expr Parser::expression() {
		return equality();
	}

	Expr Parser::equality() {
		Expr expr = comparison();
		while (match({TokenType::EqEq, TokenType::BangEq})) {
			Token op = previous();
			Expr right = comparison();
			expr =
				Expr{BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}};
		}
		return expr;
	}

	Expr Parser::comparison() {
		Expr expr = term();
		while (match({TokenType::Gt, TokenType::GtEq, TokenType::Lt, TokenType::LtEq})) {
			Token op = previous();
			Expr right = term();
			expr =
				Expr{BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}};
		}
		return expr;
	}

	Expr Parser::term() {
		Expr expr = factor();
		while (match({TokenType::Minus, TokenType::Plus})) {
			Token op = previous();
			Expr right = factor();
			expr =
				Expr{BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}};
		}
		return expr;
	}

	Expr Parser::factor() {
		Expr expr = unary();
		while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
			Token op = previous();
			Expr right = unary();
			expr =
				Expr{BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}};
		}
		return expr;
	}

	Expr Parser::unary() {
		if (match({TokenType::Bang, TokenType::Minus})) {
			const Token op = previous();
			Expr right = unary();
			return Expr{UnaryExpr{op, std::make_unique<Expr>(std::move(right))}};
		}
		return primary();
	}

	Expr Parser::primary() {
		if (match({TokenType::KwFalse}))
			return Expr{LiteralExpr{previous()}};
		if (match({TokenType::KwTrue}))
			return Expr{LiteralExpr{previous()}};
		if (match({TokenType::KwNull}))
			return Expr{LiteralExpr{previous()}};
		if (match({TokenType::Number, TokenType::String}))
			return Expr{LiteralExpr{previous()}};
		if (match({TokenType::Identifier}))
			return Expr{VariableExpr{previous()}};
		if (match({TokenType::LParen})) {
			Expr expr = expression();
			consume(TokenType::RParen, "Expect ')' after expression.");
			return Expr{GroupingExpr{std::make_unique<Expr>(std::move(expr))}};
		}
		throw std::runtime_error("Expect expression.");
	}

	Stmt Parser::declaration() {
		if (match({TokenType::KwVal, TokenType::KwVar}))
			return var_declaration();
		return statement();
	}

	Stmt Parser::var_declaration() {
		const bool is_mutable = previous().type == TokenType::KwVar;
		const Token name = consume(TokenType::Identifier, "Expect variable name.");
		std::unique_ptr<Expr> initializer = nullptr;
		if (match({TokenType::Eq}))
			initializer = std::make_unique<Expr>(expression());
		consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
		return Stmt{VarDeclStmt{name, std::move(initializer), is_mutable, {}}};
	}

	Stmt Parser::statement() {
		if (match({TokenType::LBrace}))
			return Stmt{BlockStmt{block()}};
		return expression_statement();
	}

	Stmt Parser::expression_statement() {
		Expr expr = expression();
		consume(TokenType::Semicolon, "Expect ';' after expression.");
		return Stmt{ExpressionStmt{std::move(expr)}};
	}

	std::vector<Stmt> Parser::block() {
		std::vector<Stmt> statements;
		while (!check(TokenType::RBrace) && !is_at_end())
			statements.push_back(declaration());
		consume(TokenType::RBrace, "Expect '}' after block.");
		return statements;
	}

	void Parser::synchronize() {
		advance();
		while (!is_at_end()) {
			if (previous().type == TokenType::Semicolon)
				return;
			switch (peek().type) {
			case TokenType::KwClass:
			case TokenType::KwFx:
			case TokenType::KwVar:
			case TokenType::KwVal:
			case TokenType::KwFor:
			case TokenType::KwIf:
			case TokenType::KwWhile:
			case TokenType::KwReturn:
				return;
			default:
				advance();
			}
		}
	}
} // namespace hadron::frontend
