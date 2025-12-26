#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "token.hpp"

namespace hadron::frontend {
	struct Expr;
	struct Stmt;

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
		bool is_mutable;
		char padding[7];
	};

	struct BlockStmt {
		std::vector<Stmt> statements;
	};

	struct Stmt {
		using Kind = std::variant<ExpressionStmt, VarDeclStmt, BlockStmt>;
		Kind kind;
	};
} // namespace hadron::frontend
