#pragma once

#include "ast.hpp"
#include "frontend/types.hpp"
#include "scope.hpp"

namespace hadron::frontend {
	class Semantic {
		TypeTable types;

		CompilationUnit &unit_;
		Scope::Ptr global_scope_;
		Scope::Ptr current_scope_;
		std::vector<std::string> errors_;
		const FunctionDecl *current_func_ = nullptr;

		void enter_scope();
		void exit_scope();
		void error(const Token &token, const std::string &message);

		Type resolve_type(const Type &t);

		[[nodiscard]] static bool are_types_equal(const Type &a, const Type &b);

		void analyze_stmt(const Stmt &stmt);
		std::optional<Type> analyze_expr(const Expr &expr);

	public:
		explicit Semantic(CompilationUnit &unit);

		[[nodiscard]] bool analyze();

		[[nodiscard]] const std::vector<std::string> &errors() const {
			return errors_;
		}
	};
} // namespace hadron::frontend
