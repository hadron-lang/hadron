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
