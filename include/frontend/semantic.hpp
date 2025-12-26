#pragma once

#include "ast.hpp"
#include "scope.hpp"

namespace hadron::frontend {
	class Semantic {
		CompilationUnit &unit_;
		Scope::Ptr global_scope_;
		Scope::Ptr current_scope_;
		std::vector<std::string> errors_;
		const FunctionDecl *current_func_ = nullptr;

		void enter_scope();
		void exit_scope();
		void error(const Token &token, const std::string &message);

		void analyze_stmt(const Stmt &stmt);
		void analyze_expr(const Expr &expr);

	public:
		explicit Semantic(CompilationUnit &unit);

		[[nodiscard]] bool analyze();

		[[nodiscard]] const std::vector<std::string> &errors() const {
			return errors_;
		}
	};
} // namespace hadron::frontend
