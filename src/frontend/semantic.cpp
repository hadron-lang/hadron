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
				const auto &func = std::get<FunctionDecl>(kind);
				std::vector<Type> paramTypes;
				for (const auto &[name, type] : func.params)
					paramTypes.push_back(type);
				std::shared_ptr<Type> retType = nullptr;
				if (func.return_type)
					retType = std::make_shared<Type>(*func.return_type);
				if (Type fnType = Type{FunctionType{std::move(paramTypes), std::move(retType)}};
					!current_scope_->define(std::string(func.name.text), fnType))
					error(func.name, "Function '" + std::string(func.name.text) + "' already declared.");
			}
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

	bool Semantic::are_types_equal(const Type &a, const Type &b) {
		if (a.kind.index() != b.kind.index())
			return false;

		if (std::holds_alternative<NamedType>(a.kind)) {
			const auto &[name_path, generic_args] = std::get<NamedType>(a.kind);
			const auto &[name_path1, generic_args1] = std::get<NamedType>(b.kind);
			if (name_path.size() != name_path1.size())
				return false;
			for (size_t i = 0; i < name_path.size(); ++i)
				if (name_path[i].text != name_path1[i].text)
					return false;
			return true;
		}

		if (std::holds_alternative<FunctionType>(a.kind)) {
			const auto &[params, return_type] = std::get<FunctionType>(a.kind);
			const auto &[params1, return_type1] = std::get<FunctionType>(b.kind);
			if (return_type && !return_type1)
				return false;
			if (!return_type && return_type1)
				return false;
			if (return_type && return_type1 && !are_types_equal(*return_type, *return_type1))
				return false;
			if (params.size() != params1.size())
				return false;
			for (size_t i = 0; i < params.size(); ++i) {
				if (!are_types_equal(params[i], params1[i]))
					return false;
			}
			return true;
		}
		// todo: Pointer, Slice

		return true;
	}

	Type Semantic::create_basic_type(const std::string_view name) {
		Token token{};
		token.text = name;
		return Type{NamedType{{token}, {}}};
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
					std::optional<Type> initType = std::nullopt;
					if (s.initializer)
						initType = analyze_expr(*s.initializer);
					Type finalType = create_basic_type("unknown");
					if (s.type_annotation) {
						finalType = *s.type_annotation;
						if (initType && !are_types_equal(finalType, *initType)) {
							error(s.name, "Type mismatch in variable declaration.");
						}
					} else if (initType)
						finalType = *initType;
					else {
						error(s.name, "Variable must have a type annotation or an initializer.");
						return;
					}

					if (!current_scope_->define(std::string(s.name.text), finalType))
						error(s.name, "Variable '" + std::string(s.name.text) + "' already declared in this scope.");
				},
				[&](const FunctionDecl &s) {
					const FunctionDecl *prev_func = current_func_;
					current_func_ = &s;
					enter_scope();
					for (const auto &[name, type] : s.params) {
						if (!current_scope_->define(std::string(name.text), type))
							error(name, "Duplicate parameter name '" + std::string(name.text) + "'.");
					}
					for (const auto &body_stmt : s.body)
						analyze_stmt(body_stmt);
					exit_scope();
					current_func_ = prev_func;
				},
				[&](const ReturnStmt &s) {
					if (!current_func_)
						return;
					const bool is_void = !current_func_->return_type.has_value();
					const bool has_value = (s.value != nullptr);
					if (is_void && has_value)
						error(s.keyword, "Void function should not return a value.");
					else if (!is_void) {
						if (!has_value)
							error(s.keyword, "Non-void function should return a value.");
						else {
							if (const auto valType = analyze_expr(*s.value);
								valType && !are_types_equal(*valType, *current_func_->return_type))
								error(s.keyword, "Return value type does not match function return type.");
						}
					}
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

	std::optional<Type> Semantic::analyze_expr(const Expr &expr) {
		return std::visit(
			overloaded{
				[&](const LiteralExpr &e) -> std::optional<Type> {
					if (e.value.type == TokenType::Number)
						return create_basic_type("i32");
					if (e.value.type == TokenType::String)
						return create_basic_type("String");
					if (e.value.type == TokenType::KwFalse || e.value.type == TokenType::KwTrue)
						return create_basic_type("bool");
					return std::nullopt;
				},
				[&](const VariableExpr &e) -> std::optional<Type> {
					auto sym = current_scope_->resolve(std::string(e.name.text));
					if (!sym) {
						error(e.name, "Undefined variable '" + std::string(e.name.text) + "'.");
						return std::nullopt;
					}
					return sym->type;
				},
				[&](const BinaryExpr &e) -> std::optional<Type> {
					const auto left = analyze_expr(*e.left);
					const auto right = analyze_expr(*e.right);
					if (!left || !right)
						return std::nullopt;
					if (!are_types_equal(*left, *right)) {
						error(e.op, "Type mismatch in binary expression.");
						return std::nullopt;
					}
					if (e.op.type == TokenType::EqEq || e.op.type == TokenType::Gt || e.op.type == TokenType::BangEq ||
						e.op.type == TokenType::Lt || e.op.type == TokenType::GtEq || e.op.type == TokenType::LtEq)
						return create_basic_type("bool");
					return left;
				},
				[&](const UnaryExpr &e) -> std::optional<Type> { return analyze_expr(*e.right); },
				[&](const GroupingExpr &e) -> std::optional<Type> { return analyze_expr(*e.expression); },
				[&](const CallExpr &e) -> std::optional<Type> {
					const auto calleeType = analyze_expr(*e.callee);
					if (!calleeType)
						return std::nullopt;
					if (!std::holds_alternative<FunctionType>(calleeType->kind)) {
						error(e.paren, "Can only call functions and classes.");
						return std::nullopt;
					}
					const auto &[params, return_type] = std::get<FunctionType>(calleeType->kind);
					if (e.args.size() != params.size()) {
						error(
							e.paren,
							"Expected " + std::to_string(params.size()) + " arguments but got " +
								std::to_string(e.args.size()) + "."
						);
						return std::nullopt;
					}
					for (size_t i = 0; i < e.args.size(); ++i) {
						auto argType = analyze_expr(e.args[i]);
						if (!argType)
							return std::nullopt;
						if (!are_types_equal(*argType, params[i])) {
							error(e.paren, "Argument " + std::to_string(i + 1) + " type mismatch.");
						}
					}
					if (return_type)
						return *return_type;
					return create_basic_type("void");
				},
				[](const auto &) -> std::optional<Type> { return std::nullopt; }
			},
			expr.kind
		);
	}
} // namespace hadron::frontend
