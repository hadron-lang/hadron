#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "token.hpp"
#include "types.hpp"

namespace hadron::frontend {
	struct Expr;
	struct Stmt;

	struct ModuleDecl {
		std::vector<Token> name_path;
	};

	struct ImportDecl {
		std::vector<Token> path;
		std::optional<Token> alias;
	};

	struct CompilationUnit {
		ModuleDecl module;
		std::vector<ImportDecl> imports;
		std::vector<Stmt> declarations;
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

	struct CallExpr {
		std::unique_ptr<Expr> callee;
		Token paren;
		std::vector<Expr> args;
	};

	struct Expr {
		using Kind = std::variant<LiteralExpr, VariableExpr, BinaryExpr, UnaryExpr, GroupingExpr, CallExpr>;
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
		bool is_extern;
		bool is_variadic;
		char padding[6];
	};

	struct StructField {
		Token name;
		Type type;
	};

	struct StructDecl {
		Token name;
		std::vector<StructField> fields;
	};

	struct EnumVariant {
		Token name;
		std::unique_ptr<Expr> value;
	};

	struct EnumDecl {
		Token name;
		std::vector<EnumVariant> variants;
	};

	struct BreakStmt {
		Token keyword;
	};

	struct ContinueStmt {
		Token keyword;
	};

	struct LoopStmt {
		std::unique_ptr<Stmt> body;
	};

	struct ForStmt {
		std::unique_ptr<Stmt> initializer;
		std::unique_ptr<Expr> condition;
		std::unique_ptr<Expr> increment;
		std::unique_ptr<Stmt> body;
	};

	struct Stmt {
		using Kind = std::variant<
			ExpressionStmt,
			VarDeclStmt,
			BlockStmt,
			IfStmt,
			WhileStmt,
			ReturnStmt,
			FunctionDecl,
			StructDecl,
			EnumDecl,
			BreakStmt,
			ContinueStmt,
			LoopStmt,
			ForStmt>;
		Kind kind;
	};
} // namespace hadron::frontend
