#include "frontend/semantic.hpp"
#include <iostream>
#include <variant>

namespace hadron::frontend {
	template <class... Ts> struct overloaded : Ts... {
		using Ts::operator()...;
	};
	template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	Semantic::Semantic(CompilationUnit &unit) : unit_(unit) {
		global_scope_ = std::make_shared<Scope>();
		current_scope_ = global_scope_;

		global_scope_->define_type("byte", types.byte);
		global_scope_->define_type("i8", types.i8);
		global_scope_->define_type("i16", types.i16);
		global_scope_->define_type("i32", types.i32);
		global_scope_->define_type("i64", types.i64);
		global_scope_->define_type("i128", types.i128);
		global_scope_->define_type("u8", types.u8);
		global_scope_->define_type("u16", types.u16);
		global_scope_->define_type("u32", types.u32);
		global_scope_->define_type("u64", types.u64);
		global_scope_->define_type("u128", types.u128);
		global_scope_->define_type("f32", types.f32);
		global_scope_->define_type("f64", types.f64);
		global_scope_->define_type("f128", types.f128);
		global_scope_->define_type("bool", types.bool_);
		global_scope_->define_type("str", types.str);
		global_scope_->define_type("void", types.void_);
	}

	bool Semantic::analyze() {
		for (const auto &[kind] : unit_.declarations) {
			if (std::holds_alternative<FunctionDecl>(kind)) {
				const auto &func = std::get<FunctionDecl>(kind);
				std::vector<Type> paramTypes;
				for (const auto &[name, type] : func.params)
					paramTypes.push_back(resolve_type(type));
				auto ret = std::make_shared<Type>(func.return_type ? resolve_type(*func.return_type) : types.void_);
				Type fnType = Type{FunctionType{std::move(paramTypes), ret}};
				if (!current_scope_->define_value(std::string(func.name.text), fnType))
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
	auto print_type = [](const Type &t) {
		std::visit(overloaded{
			[](BuiltinType bt){ std::cout << "BuiltinType(" << static_cast<int>(bt) << ")"; },
			[](const NamedType &nt){ 
				std::cout << "NamedType(";
				for (auto &tok : nt.name_path) std::cout << tok.text << ".";
				std::cout << ")";
			},
			[](const FunctionType &ft){ 
				std::cout << "FunctionType(params=[";
				for (auto &p : ft.params) std::visit(overloaded{
					[](BuiltinType bt){ std::cout << static_cast<int>(bt) << ","; },
					[](auto&){ std::cout << "?,"; }
				}, p.kind);
				std::cout << "], return=";
				if (ft.return_type)
					std::visit(overloaded{
						[](BuiltinType bt){ std::cout << static_cast<int>(bt); },
						[](auto&){ std::cout << "?"; }
					}, ft.return_type->kind);
				else
					std::cout << "void";
				std::cout << ")";
			},
			[](const PointerType &pt){ std::cout << "PointerType(?)"; },
			[](const SliceType &st){ std::cout << "SliceType(?)"; },
			[](const ErrorType &){ std::cout << "ErrorType"; }
		}, t.kind);
	};

	std::cout << "Comparing types: ";
	print_type(a);
	std::cout << " vs ";
	print_type(b);
	std::cout << std::endl;

		if (std::holds_alternative<ErrorType>(a.kind) || std::holds_alternative<ErrorType>(b.kind))
			return true;

		if (a.kind.index() != b.kind.index())
			return false;

		if (auto ba = std::get_if<BuiltinType>(&a.kind))
			return *ba == std::get<BuiltinType>(b.kind);

		if (auto fa = std::get_if<FunctionType>(&a.kind)) {
			const auto &fb = std::get<FunctionType>(b.kind);
			if (fa->params.size() != fb.params.size())
				return false;
			for (size_t i = 0; i < fa->params.size(); ++i)
				if (!are_types_equal(fa->params[i], fb.params[i]))
					return false;
			return are_types_equal(*fa->return_type, *fb.return_type);
		}
		// todo: Pointer, Slice

		return false;
	}




	Type Semantic::resolve_type(const Type &t) {
		return std::visit(overloaded {
			[&](const BuiltinType &b) -> Type {
				return t;
			},
			[&](const NamedType &n) -> Type {
				if (n.name_path.empty())
					return types.error;
				const std::string type_name = std::string(n.name_path[0].text);
				if (auto t = current_scope_->resolve_type(type_name))
					return *t;
				error(n.name_path[0], "Unknown type '" + type_name + "'");
				return types.error;
			},
			[&](const PointerType &p) -> Type {
				Type inner = resolve_type(*p.inner);
				return Type{PointerType{std::make_unique<Type>(std::move(inner))}};
			},
			[&](const SliceType &s) -> Type {
				Type inner = resolve_type(*s.inner);
				return Type{SliceType{std::make_unique<Type>(std::move(inner))}};
			},
			[&](const FunctionType &f) -> Type {
				return Type{f}; // f is already resolved
			},
			[&](const ErrorType &e) -> Type {
				return types.error;
			}
		}, t.kind);
	}

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
					Type finalType = types.error;
					if (s.type_annotation) {
						finalType = resolve_type(*s.type_annotation);
						if (initType && !are_types_equal(finalType, *initType)) {
							error(s.name, "Type mismatch in variable declaration.");
						}
					} else if (initType)
						finalType = *initType;
					else {
						error(s.name, "Variable must have a type annotation or an initializer.");
						return;
					}

					if (!current_scope_->define_value(std::string(s.name.text), finalType))
						error(s.name, "Variable '" + std::string(s.name.text) + "' already declared in this scope.");
				},
				[&](const FunctionDecl &s) {
					const FunctionDecl *prev_func = current_func_;
					current_func_ = &s;
					enter_scope();
					for (const auto &[name, type] : s.params) {
						if (!current_scope_->define_value(std::string(name.text), resolve_type(type)))
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
								valType && !are_types_equal(*valType, resolve_type(*current_func_->return_type)))
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
						return types.i32;
					if (e.value.type == TokenType::String)
						return types.str;
					if (e.value.type == TokenType::KwFalse || e.value.type == TokenType::KwTrue)
						return types.bool_;
					return std::nullopt;
				},
				[&](const VariableExpr &e) -> std::optional<Type> {
					auto sym = current_scope_->resolve_value(std::string(e.name.text));
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
						return types.bool_;
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
					return types.void_;
				},
				[](const auto &) -> std::optional<Type> { return std::nullopt; }
			},
			expr.kind
		);
	}
} // namespace hadron::frontend
