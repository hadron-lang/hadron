#pragma once

#include "ast.hpp"
#include "scope.hpp"

namespace hadron::frontend {
	class Semantic {
		const CompilationUnit &unit_;
		Scope *currentScope_{};
		std::vector<SemanticError> errors_{};

		void visit(const VarDeclStmt &);
		void visit(const BlockStmt &);
		void visit(const ExpressionStmt &);
		void visit(const IfStmt &);
		void visit(const WhileStmt &);
		void visit(const ReturnStmt &);
		void visit(const FunctionDecl &);
		void visit(const Stmt &);
		void visit(const Expr &);
		void visit(const Param &) const;
		void visit(const LiteralExpr &);
		void visit(const VariableExpr &);
		void visit(const BinaryExpr &);
		void visit(const UnaryExpr &);
		void visit(const GroupingExpr &);

	public:
		explicit Semantic(const CompilationUnit &unit);

		[[nodiscard]] std::vector<SemanticError> analyze();
	};
}
