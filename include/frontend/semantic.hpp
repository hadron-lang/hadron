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

		[[nodiscard]] static bool are_types_equal(const Type &a, const Type &b);
		[[nodiscard]] static Type create_basic_type(std::string_view name);

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
