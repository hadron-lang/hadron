#include <iostream>
#include <stdexcept>

#include "frontend/parser.hpp"

#include <algorithm>

namespace hadron::frontend {
	Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

	CompilationUnit Parser::parse() {
		if (!check(TokenType::KwModule))
			throw std::runtime_error("Expect 'module' declaration at top of file.");

		ModuleDecl module = parse_module();
		std::vector<ImportDecl> imports;
		while (check(TokenType::KwImport))
			imports.push_back(parse_import());

		std::vector<Stmt> declarations;
		while (!is_at_end()) {
			try {
				declarations.push_back(top_level_declaration());
			} catch (const std::runtime_error &error) {
				std::cerr << "Parse Error: " << error.what() << "\n";
				synchronize();
			}
		}

		return CompilationUnit{std::move(module), std::move(imports), std::move(declarations)};
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
		if (std::ranges::any_of(types, [this](const TokenType type) { return check(type); })) {
			advance();
			return true;
		}
		return false;
	}

	Token Parser::consume(const TokenType type, const std::string_view message) {
		if (check(type))
			return advance();
		throw std::runtime_error(std::string(message) + " at " + peek().to_string());
	}

	std::vector<Token> Parser::parse_qualified_name() {
		std::vector<Token> path;
		do {
			path.push_back(consume(TokenType::Identifier, "Expect identifier in path."));
		} while (match({TokenType::Dot}));
		return path;
	}

	ModuleDecl Parser::parse_module() {
		consume(TokenType::KwModule, "Expect 'module'.");
		std::vector<Token> name_path = parse_qualified_name();
		consume(TokenType::Semicolon, "Expect ';' after module name.");
		return ModuleDecl{std::move(name_path)};
	}

	ImportDecl Parser::parse_import() {
		consume(TokenType::KwImport, "Expect 'import'.");
		std::vector<Token> path = parse_qualified_name();

		std::optional<Token> alias;
		if (check(TokenType::KwAs)) {
			advance();
			alias = consume(TokenType::Identifier, "Expect alias after 'as'.");
		}

		consume(TokenType::Semicolon, "Expect ';' after import.");
		return ImportDecl{std::move(path), alias};
	}

	Stmt Parser::top_level_declaration() {
		// todo: add 'public' and 'private'
		if (check(TokenType::KwExtern)) {
			advance();
			if (match({TokenType::KwFx}))
				return function_declaration(true);
			throw std::runtime_error("Expect 'fx' after 'extern'.");
		}
		if (match({TokenType::KwFx}))
			return function_declaration(false);
		if (match({TokenType::KwVar, TokenType::KwVal}))
			return var_declaration();
		if (check(TokenType::KwStruct))
			return struct_declaration();
		if (check(TokenType::KwEnum))
			return enum_declaration();
		throw std::runtime_error("Expect top-level declaration (fx, struct, etc.). Found: " + std::string(peek().text));
	}

	Type Parser::parse_type() {
		const Token name = consume(TokenType::Identifier, "Expect type name");

		if (name.text == "ptr") {
			consume(TokenType::Lt, "Expect '<' after 'ptr'.");
			Type inner = parse_type();
			consume(TokenType::Gt, "Expect '>' after type argument.");
			return Type{PointerType{std::make_unique<Type>(std::move(inner))}};
		}

		if (name.text == "slice") {
			consume(TokenType::Lt, "Expect '<' after 'ptr'.");
			Type inner = parse_type();
			consume(TokenType::Gt, "Expect '>' after type argument.");
			return Type{SliceType{std::make_unique<Type>(std::move(inner))}};
		}

		std::vector<Token> path;
		path.push_back(name);
		while (match({TokenType::Dot}))
			path.push_back(consume(TokenType::Identifier, "Expect identifier after '.'."));

		std::vector<Type> generics;
		if (match({TokenType::Lt})) {
			do {
				generics.push_back(parse_type());
			} while (match({TokenType::Comma}));
			consume(TokenType::Gt, "Expect '<' after generic arguments.");
		}

		return Type{NamedType{std::move(path), std::move(generics)}};
	}

	Expr Parser::assignment() {
		Expr expr = logical_or();

		if (match({TokenType::Eq, TokenType::PlusEq, TokenType::MinusEq, TokenType::StarEq, TokenType::SlashEq})) {
			const Token equals = previous();
			Expr value = assignment();

			return Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), equals, std::make_unique<Expr>(std::move(value))},
				{}
			};
		}

		return expr;
	}

	Expr Parser::expression() {
		return assignment();
	}

	Expr Parser::equality() {
		Expr expr = comparison();
		while (match({TokenType::EqEq, TokenType::BangEq})) {
			const Token op = previous();
			Expr right = comparison();
			expr = Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}, {}
			};
		}
		return expr;
	}

	Expr Parser::logical_or() {
		Expr expr = logical_and();
		while (match({TokenType::Or})) {
			const Token op = previous();
			Expr right = logical_and();
			expr = Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}, {}
			};
		}
		return expr;
	}

	Expr Parser::logical_and() {
		Expr expr = equality();
		while (match({TokenType::And})) {
			const Token op = previous();
			Expr right = equality();
			expr = Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}, {}
			};
		}
		return expr;
	}

	Expr Parser::comparison() {
		Expr expr = term();
		while (match({TokenType::Gt, TokenType::GtEq, TokenType::Lt, TokenType::LtEq})) {
			const Token op = previous();
			Expr right = term();
			expr = Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}, {}
			};
		}
		return expr;
	}

	Expr Parser::term() {
		Expr expr = factor();
		while (match({TokenType::Minus, TokenType::Plus})) {
			const Token op = previous();
			Expr right = factor();
			expr = Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}, {}
			};
		}
		return expr;
	}

	Expr Parser::factor() {
		Expr expr = cast();
		while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
			const Token op = previous();
			Expr right = cast();
			expr = Expr{
				BinaryExpr{std::make_unique<Expr>(std::move(expr)), op, std::make_unique<Expr>(std::move(right))}, {}
			};
		}
		return expr;
	}

	Expr Parser::cast() {
		Expr expr = unary();
		while (match({TokenType::KwAs})) {
			const Token op = previous();
			Type type = parse_type();
			expr = Expr{CastExpr{std::make_unique<Expr>(std::move(expr)), op, std::move(type)}, {}};
		}
		return expr;
	}

	Expr Parser::unary() {
		if (match({TokenType::Bang, TokenType::Minus, TokenType::Ampersand, TokenType::Star})) {
			const Token op = previous();
			Expr right = unary();
			return Expr{UnaryExpr{op, std::make_unique<Expr>(std::move(right))}, {}};
		}
		return call();
	}

	Expr Parser::call() {
		Expr expr = primary();
		while (true) {
			if (match({TokenType::LParen}))
				expr = finish_call(std::move(expr));
			else if (match({TokenType::Dot})) {
				const Token name = consume(TokenType::Identifier, "Expect property name after '.'.");
				expr = Expr{GetExpr{std::make_unique<Expr>(std::move(expr)), name}, {}};
			} else if (match({TokenType::LBracket})) {
				Expr index = expression();
				const Token rBracket = consume(TokenType::RBracket, "");
				expr = Expr{
					ArrayAccessExpr{
						std::make_unique<Expr>(std::move(expr)), std::make_unique<Expr>(std::move(index)), rBracket
					},
					{}
				};
			} else
				break;
		}
		return expr;
	}

	Expr Parser::finish_call(Expr callee) {
		std::vector<Expr> arguments;
		if (!check(TokenType::RParen)) {
			do {
				if (arguments.size() >= 255) {
				}
				arguments.push_back(expression());
			} while (match({TokenType::Comma}));
		}

		const Token paren = consume(TokenType::RParen, "Expect ')' after arguments.");
		return Expr{CallExpr{std::make_unique<Expr>(std::move(callee)), paren, std::move(arguments)}, {}};
	}

	Expr Parser::primary() {
		if (match({TokenType::KwFalse}))
			return Expr{LiteralExpr{previous()}, {}};
		if (match({TokenType::KwTrue}))
			return Expr{LiteralExpr{previous()}, {}};
		if (match({TokenType::KwNull}))
			return Expr{LiteralExpr{previous()}, {}};
		if (match({TokenType::Number, TokenType::String}))
			return Expr{LiteralExpr{previous()}, {}};
		if (match({TokenType::Identifier})) {
			if (check(TokenType::LBrace)) {
				Token typeName = previous();
				Type type = Type{NamedType{{typeName}, {}}};
				Token lBrace = consume(TokenType::LBrace, "Expect '{' after structure name.");
				std::vector<FieldInit> fields;
				if (!check(TokenType::RBrace)) {
					do {
						Token fieldName = consume(TokenType::Identifier, "Expect field name.");
						consume(TokenType::Eq, "Expect '=' after field name.");
						Expr value = expression();
						fields.emplace_back(FieldInit{fieldName, std::make_unique<Expr>(std::move(value))});
					} while (match({TokenType::Comma}));
				}
				consume(TokenType::RBrace, "Expect '}' after struct fields.");
				return Expr{StructInitExpr{std::move(type), lBrace, std::move(fields)}, {}};
			}

			return Expr{VariableExpr{previous()}, {}};
		}
		if (match({TokenType::KwSizeOf})) {
			const Token keyword = previous();
			consume(TokenType::LParen, "Expect '(' after 'sizeof'.");
			const Type type = parse_type();
			consume(TokenType::RParen, "Expect ')' after type.");
			return Expr{SizeOfExpr{keyword, type}, {}};
		}
		if (match({TokenType::LParen})) {
			const Token paren = previous();
			Expr expr = expression();
			consume(TokenType::RParen, "Expect ')' after expression.");
			return Expr{GroupingExpr{std::make_unique<Expr>(std::move(expr)), paren}, {}};
		}
		throw std::runtime_error("Expect expression.");
	}

	Stmt Parser::declaration() {
		if (check(TokenType::KwExtern)) {
			advance();
			if (match({TokenType::KwFx}))
				return function_declaration(true);
			throw std::runtime_error("Expect 'fx' after 'extern'.");
		}
		if (match({TokenType::KwFx}))
			return function_declaration(false);
		if (match({TokenType::KwVal, TokenType::KwVar}))
			return var_declaration();
		return statement();
	}

	Stmt Parser::var_declaration() {
		const bool is_mutable = previous().type == TokenType::KwVar;
		const Token name = consume(TokenType::Identifier, "Expect variable name.");

		std::optional<Type> type_annotation;
		if (match({TokenType::Colon}))
			type_annotation = parse_type();

		std::unique_ptr<Expr> initializer = nullptr;
		if (match({TokenType::Eq}))
			initializer = std::make_unique<Expr>(expression());
		consume(TokenType::Semicolon, "Expect ';' after variable declaration.");

		return Stmt{VarDeclStmt{name, std::move(initializer), std::move(type_annotation), is_mutable, {}}};
	}

	std::vector<Param> Parser::parse_params() {
		std::vector<Param> params;
		if (!check(TokenType::RParen)) {
			do {
				const Token name = consume(TokenType::Identifier, "Expect parameter name.");
				consume(TokenType::Colon, "Expect ':' after parameter name.");
				Type type = parse_type();
				params.push_back(Param{name, std::move(type)});
			} while (match({TokenType::Comma}));
		}
		return params;
	}

	Stmt Parser::function_declaration(const bool is_extern) {
		const Token name = consume(TokenType::Identifier, "Expect function name.");

		consume(TokenType::LParen, "Expect '(' after function name.");

		std::vector<Param> params;
		bool is_variadic = false;
		if (!check(TokenType::RParen)) {
			do {
				if (check(TokenType::Ellipsis)) {
					consume(TokenType::Ellipsis, "Expect '...'");
					is_variadic = true;
					break;
				}

				const Token paramName = consume(TokenType::Identifier, "Expect parameter name.");
				consume(TokenType::Colon, "Expect ':' after parameter name.");
				Type type = parse_type();
				params.push_back(Param{paramName, std::move(type)});
			} while (match({TokenType::Comma}));
		}
		consume(TokenType::RParen, "Expect ')' after parameters.");

		std::optional<Type> return_type;
		if (check(TokenType::Identifier))
			return_type = parse_type();

		std::vector<Stmt> body;
		if (is_extern)
			consume(TokenType::Semicolon, "Expect ';' after extern function declaration.");
		else {
			consume(TokenType::LBrace, "Expect '{' before function body.");
			body = block();
		}

		return Stmt{
			FunctionDecl{name, std::move(params), std::move(return_type), std::move(body), is_extern, is_variadic, {}}
		};
	}

	Stmt Parser::statement() {
		if (check(TokenType::KwBreak))
			return break_statement();
		if (check(TokenType::KwContinue))
			return continue_statement();
		if (check(TokenType::KwLoop))
			return loop_statement();
		if (check(TokenType::KwFor))
			return for_statement();
		if (match({TokenType::KwReturn}))
			return return_statement();
		if (match({TokenType::KwIf}))
			return if_statement();
		if (match({TokenType::KwWhile}))
			return while_statement();
		if (match({TokenType::LBrace}))
			return Stmt{BlockStmt{block()}};
		return expression_statement();
	}

	Stmt Parser::expression_statement() {
		Expr expr = expression();
		consume(TokenType::Semicolon, "Expect ';' after expression.");
		return Stmt{ExpressionStmt{std::move(expr)}};
	}

	Stmt Parser::if_statement() {
		consume(TokenType::LParen, "Expect '(' after 'if'.");
		Expr condition = expression();
		consume(TokenType::RParen, "Expect ')' after if condition.");

		Stmt thenBranch = parse_block_stmt();
		std::unique_ptr<Stmt> elseBranch = nullptr;
		if (match({TokenType::KwElse})) {
			if (check(TokenType::KwIf)) {
				advance();
				elseBranch = std::make_unique<Stmt>(if_statement());
			} else
				elseBranch = std::make_unique<Stmt>(parse_block_stmt());
		}

		return Stmt{IfStmt{std::move(condition), std::make_unique<Stmt>(std::move(thenBranch)), std::move(elseBranch)}};
	}

	Stmt Parser::while_statement() {
		consume(TokenType::LParen, "Expect '(' after 'while'.");
		Expr condition = expression();
		consume(TokenType::RParen, "Expect ')' after while condition.");
		Stmt body = parse_block_stmt();
		return Stmt{WhileStmt{std::move(condition), std::make_unique<Stmt>(std::move(body))}};
	}

	Stmt Parser::loop_statement() {
		consume(TokenType::KwLoop, "Expect 'loop'.");
		Stmt body = parse_block_stmt();
		return Stmt{LoopStmt{std::make_unique<Stmt>(std::move(body))}};
	}

	Stmt Parser::for_statement() {
		consume(TokenType::KwFor, "Expect 'for'.");
		consume(TokenType::LParen, "Expect '(' after 'for'.");

		std::unique_ptr<Stmt> initializer = nullptr;
		if (match({TokenType::Semicolon})) {
		} else if (match({TokenType::KwVar, TokenType::KwVal}))
			initializer = std::make_unique<Stmt>(var_declaration());
		else
			initializer = std::make_unique<Stmt>(expression_statement());

		std::unique_ptr<Expr> condition = nullptr;
		if (!check(TokenType::Semicolon))
			condition = std::make_unique<Expr>(expression());

		consume(TokenType::Semicolon, "Expect ';' after loop condition.");

		std::unique_ptr<Expr> increment = nullptr;
		if (!check(TokenType::RParen))
			increment = std::make_unique<Expr>(expression());

		consume(TokenType::RParen, "Expect ')' after for clauses.");
		Stmt body = parse_block_stmt();

		return Stmt{ForStmt{
			std::move(initializer), std::move(condition), std::move(increment), std::make_unique<Stmt>(std::move(body))
		}};
	}

	Stmt Parser::break_statement() {
		const Token keyword = consume(TokenType::KwBreak, "Expect 'break'.");
		consume(TokenType::Semicolon, "Expect ';' after break.");
		return Stmt{BreakStmt{keyword}};
	}

	Stmt Parser::continue_statement() {
		const Token keyword = consume(TokenType::KwContinue, "Expect 'continue'.");
		consume(TokenType::Semicolon, "Expect ';' after 'continue'.");
		return Stmt{ContinueStmt{keyword}};
	}

	Stmt Parser::return_statement() {
		const Token keyword = previous();
		std::unique_ptr<Expr> value = nullptr;
		if (!check(TokenType::Semicolon))
			value = std::make_unique<Expr>(expression());
		consume(TokenType::Semicolon, "Expect ';' after return value.");
		return Stmt{ReturnStmt{keyword, std::move(value)}};
	}

	Stmt Parser::struct_declaration() {
		consume(TokenType::KwStruct, "Expect 'struct' keyword.");
		const Token name = consume(TokenType::Identifier, "Expect struct name.");
		consume(TokenType::LBrace, "Expect '{' before struct body.");

		std::vector<StructField> fields;
		while (!check(TokenType::RBrace) && !is_at_end()) {
			const Token fieldName = consume(TokenType::Identifier, "Expect field name.");
			consume(TokenType::Colon, "Expect ':' after field name.");
			Type fieldType = parse_type();
			consume(TokenType::Semicolon, "Expect ';' after field declaration.");
			fields.push_back(StructField{fieldName, std::move(fieldType)});
		}
		consume(TokenType::RBrace, "Expect '}' after struct body.");
		return Stmt{StructDecl{name, std::move(fields)}};
	}

	Stmt Parser::enum_declaration() {
		consume(TokenType::KwEnum, "Expect 'enum' keyword.");
		const Token name = consume(TokenType::Identifier, "Expect enum name.");
		consume(TokenType::LBrace, "Expect '{' before enum variants.");

		std::vector<EnumVariant> variants;
		while (!check(TokenType::RBrace) && !is_at_end()) {
			const Token variantName = consume(TokenType::Identifier, "Expect enum variant name.");
			std::unique_ptr<Expr> value = nullptr;
			if (match({TokenType::Eq}))
				value = std::make_unique<Expr>(expression());
			variants.push_back(EnumVariant{variantName, std::move(value)});
			if (!check(TokenType::RBrace))
				consume(TokenType::Comma, "Expect ',' between enum variants.");
			else
				match({TokenType::Comma});
		}

		consume(TokenType::RBrace, "Expect '}' after enum variants.");
		return Stmt{EnumDecl{name, std::move(variants)}};
	}

	std::vector<Stmt> Parser::block() {
		std::vector<Stmt> statements;
		while (!check(TokenType::RBrace) && !is_at_end())
			statements.push_back(declaration());
		consume(TokenType::RBrace, "Expect '}' after block.");
		return statements;
	}

	Stmt Parser::parse_block_stmt() {
		consume(TokenType::LBrace, "Expect '{' before block.");
		return Stmt{BlockStmt{block()}};
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
