#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "token.hpp"

namespace hadron::frontend {
	struct Type;
	struct Expr;
	struct Stmt;

	struct NamedType {
		std::vector<Token> name_path;
		std::vector<Type> generic_args;
	};

	struct PointerType {
		std::unique_ptr<Type> inner;
	};

	struct SliceType {
		std::unique_ptr<Type> inner;
	};

	struct Type {
		using Kind = std::variant<NamedType, PointerType, SliceType>;
		Kind kind;
	};

	struct LiteralExpr {
		Token value;
	};

	struct VariableExpr {
		Token name;
	};

	struct BinaryExpr {
		std::unique_ptr<Expr> left;
		Token op;
		std::unique_ptr<Expr> right;
	};

	struct UnaryExpr {
		Token op;
		std::unique_ptr<Expr> right;
	};

	struct GroupingExpr {
		std::unique_ptr<Expr> expression;
	};

	struct Expr {
		using Kind = std::variant<LiteralExpr, VariableExpr, BinaryExpr, UnaryExpr, GroupingExpr>;
		Kind kind;
	};

	struct ExpressionStmt {
		Expr expression;
	};

	struct VarDeclStmt {
		Token name;
		std::unique_ptr<Expr> initializer;
		std::optional<Type> type_annotation;
		bool is_mutable;
		char padding[15];
	};

	struct BlockStmt {
		std::vector<Stmt> statements;
	};

	struct IfStmt {
		Expr condition;
		std::unique_ptr<Stmt> then_branch;
		std::unique_ptr<Stmt> else_branch;
	};

	struct WhileStmt {
		Expr condition;
		std::unique_ptr<Stmt> body;
	};

	struct Param {
		Token name;
		Type type;
	};

	struct ReturnStmt {
		Token keyword;
		std::unique_ptr<Expr> value;
	};

	struct FunctionDecl {
		Token name;
		std::vector<Param> params;
		std::optional<Type> return_type;
		std::vector<Stmt> body;
	};

	struct Stmt {
		using Kind = std::variant<ExpressionStmt, VarDeclStmt, BlockStmt, IfStmt, WhileStmt, ReturnStmt, FunctionDecl>;
		Kind kind;
	};
} // namespace hadron::frontend
