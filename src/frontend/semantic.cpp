#include <format>
#include <iostream>

#include "frontend/semantic.hpp"

namespace hadron::frontend {
	Semantic::Semantic(CompilationUnit &unit) : unit_(unit) {
		global_scope_ = std::make_shared<Scope>();
		current_scope_ = global_scope_;
	}

	bool Semantic::analyze() {
		for (const auto &[kind] : unit_.declarations) {
			if (std::holds_alternative<FunctionDecl>(kind)) {
				if (const auto &func = std::get<FunctionDecl>(kind);
					!current_scope_->define(std::string(func.name.text), "fn"))
					error(func.name, "Function '" + std::string(func.name.text) + "' already declared.");
			}
			// todo: structs, enums
		}

		for (const auto &stmt : unit_.declarations)
			analyze_stmt(stmt);

		return errors_.empty();
	}

	void Semantic::enter_scope() {
		current_scope_ = std::make_shared<Scope>(current_scope_);
	}

	void Semantic::exit_scope() {
		current_scope_ = current_scope_->parent();
	}

	void Semantic::error(const Token &token, const std::string &message) {
		char *err = static_cast<char *>(__builtin_alloca(256));
		snprintf(err, 256, "%d:%d Semantic Error: %s", token.location.line, token.location.column, message.c_str());
		errors_.push_back(std::string(err));
	}

	template <class... Ts> struct overloaded : Ts... {
		using Ts::operator()...;
	};
	template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	void Semantic::analyze_stmt(const Stmt &stmt) {
		std::visit(
			overloaded{
				[&](const BlockStmt &s) {
					enter_scope();
					for (const auto &sub_stmt : s.statements)
						analyze_stmt(sub_stmt);
					exit_scope();
				},
				[&](const VarDeclStmt &s) {
					if (s.initializer)
						analyze_expr(*s.initializer);
					if (!current_scope_->define(std::string(s.name.text), "var"))
						error(s.name, "Variable '" + std::string(s.name.text) + "' already declared in this scope.");
				},
				[&](const FunctionDecl &s) {
					const FunctionDecl *prev_func = current_func_;
					current_func_ = &s;
					enter_scope();
					for (const auto &[name, type] : s.params) {
						if (!current_scope_->define(std::string(name.text), "param"))
							error(name, "Duplicate parameter name '" + std::string(name.text) + "'.");
					}
					for (const auto &body_stmt : s.body)
						analyze_stmt(body_stmt);
					exit_scope();
					current_func_ = prev_func;
				},
				[&](const ReturnStmt &s) {
					const bool is_void = !current_func_->return_type.has_value();
					const bool has_value = (s.value != nullptr);

					if (is_void && has_value)
						error(s.keyword, "Void function should not return a value.");
					else if (!is_void && !has_value)
						error(s.keyword, "Non-void function should return a value.");
					if (s.value)
						analyze_expr(*s.value);
				},
				[&](const IfStmt &s) {
					analyze_expr(s.condition);
					if (s.then_branch)
						analyze_stmt(*s.then_branch);
					if (s.else_branch)
						analyze_stmt(*s.else_branch);
				},
				[&](const WhileStmt &s) {
					analyze_expr(s.condition);
					if (s.body)
						analyze_stmt(*s.body);
				},
				[&](const LoopStmt &s) {
					if (s.body)
						analyze_stmt(*s.body);
				},
				[&](const ForStmt &s) {
					enter_scope();
					if (s.initializer)
						analyze_stmt(*s.initializer);
					if (s.condition)
						analyze_expr(*s.condition);
					if (s.increment)
						analyze_expr(*s.increment);
					if (s.body)
						analyze_stmt(*s.body);
					exit_scope();
				},
				[&](const ExpressionStmt &s) { analyze_expr(s.expression); },
				[](const auto &) {}
			},
			stmt.kind
		);
	}

	void Semantic::analyze_expr(const Expr &expr) {
		std::visit(
			overloaded{
				[&](const VariableExpr &e) {
					if (!current_scope_->resolve(std::string(e.name.text)))
						error(e.name, "Undefined variable '" + std::string(e.name.text) + "'.");
				},
				[&](const BinaryExpr &e) {
					analyze_expr(*e.left);
					analyze_expr(*e.right);
				},
				[&](const UnaryExpr &e) { analyze_expr(*e.right); },
				[&](const GroupingExpr &e) { analyze_expr(*e.expression); },
				[](const auto &) {}
			},
			expr.kind
		);
	}
} // namespace hadron::frontend
