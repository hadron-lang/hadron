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
		global_scope_->define_type("u8", types.u8);
		global_scope_->define_type("u16", types.u16);
		global_scope_->define_type("u32", types.u32);
		global_scope_->define_type("u64", types.u64);
		global_scope_->define_type("usize", types.u64);
		global_scope_->define_type("f32", types.f32);
		global_scope_->define_type("f64", types.f64);
		global_scope_->define_type("bool", types.bool_);
		global_scope_->define_type("void", types.void_);
	}

	bool Semantic::analyze() {
		for (const auto &[kind] : unit_.declarations) {
			if (std::holds_alternative<StructDecl>(kind)) {
				const auto &[name, fields] = std::get<StructDecl>(kind);
				if (Type structTy = Type{StructType{std::string(name.text), {}}};
					!current_scope_->define_type(std::string(name.text), structTy))
					error(name, "Type '" + std::string(name.text) + "' already declared.");
			}
		}

		for (const auto &[kind] : unit_.declarations) {
			if (std::holds_alternative<StructDecl>(kind)) {
				const auto &[name, fields] = std::get<StructDecl>(kind);
				std::vector<std::pair<std::string, Type>> vecFields;
				for (const auto &[token, type] : fields)
					vecFields.emplace_back(token.text, resolve_type(type));
				Type structTy = Type{StructType{std::string(name.text), std::move(vecFields)}};
				current_scope_->redefine_type(std::string(name.text), structTy);
			} else if (std::holds_alternative<FunctionDecl>(kind)) {
				const auto &func = std::get<FunctionDecl>(kind);
				std::vector<Type> paramTypes;
				for (const auto &[name, type] : func.params)
					paramTypes.push_back(resolve_type(type));
				const auto ret =
					std::make_shared<Type>(func.return_type ? resolve_type(*func.return_type) : types.void_);
				if (Type fnType = Type{FunctionType{std::move(paramTypes), ret, func.is_variadic, {}}};
					!current_scope_->define_value(std::string(func.name.text), fnType))
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
		char *err = static_cast<char *>(__builtin_alloca(512));
		snprintf(err, 512, "%d:%d Semantic Error: %s", token.location.line, token.location.column, message.c_str());
		errors_.emplace_back(err);
	}

	Type Semantic::resolve_type(const Type &t) {
		return std::visit(
			overloaded{
				[&](const BuiltinType &b) -> Type { return t; },
				[&](const NamedType &n) -> Type {
					if (n.name_path.empty())
						return types.error;
					const std::string type_name = std::string(n.name_path[0].text);
					if (auto tp = current_scope_->resolve_type(type_name)) {
						if (std::holds_alternative<StructType>(tp->kind))
							return t;
						return *tp;
					}
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
				[&](const FunctionType &f) -> Type { return Type{f}; },
				[&](const StructType &s) -> Type { return t; },
				[&](const ErrorType &e) -> Type { return types.error; }
			},
			t.kind
		);
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
							bool allowed = false;
							const Token *litToken = nullptr;
							bool isNeg = false;

							if (s.initializer) {
								if (std::holds_alternative<LiteralExpr>(s.initializer->kind)) {
									const auto &[value] = std::get<LiteralExpr>(s.initializer->kind);
									if (value.type == TokenType::Number) {
										litToken = &value;
										isNeg = false;
									}
								} else if (std::holds_alternative<UnaryExpr>(s.initializer->kind)) {
									if (const auto &[op, right] = std::get<UnaryExpr>(s.initializer->kind);
										op.type == TokenType::Minus &&
										std::holds_alternative<LiteralExpr>(right->kind)) {
										const auto &[value] = std::get<LiteralExpr>(right->kind);
										if (value.type == TokenType::Number) {
											litToken = &value;
											isNeg = true;
										}
									}
								}
							}

							if (litToken && is_integer_type(finalType)) {
								if (check_int_literal(std::string(litToken->text), finalType, isNeg))
									allowed = true;
								else {
									error(*litToken, "Integer literal out of range for type.");
									return;
								}
							}

							if (!allowed)
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
					if (s.is_extern)
						return;
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
					if (const bool has_value = (s.value != nullptr); is_void && has_value)
						error(s.keyword, "Void function should not return a value.");
					else if (!is_void) {
						if (!has_value)
							error(s.keyword, "Non-void function should return a value.");
						else {
							const auto valType = analyze_expr(*s.value);
							if (!valType)
								return;

							if (const auto expectedType = resolve_type(*current_func_->return_type);
								!are_types_equal(*valType, expectedType)) {
								if (is_integer_type(*valType) && is_integer_type(expectedType)) {
								} else
									error(s.keyword, "Return value type does not match function return type.");
							}
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
		auto result = std::visit(
			overloaded{
				[&](const LiteralExpr &e) -> std::optional<Type> {
					if (e.value.type == TokenType::Number)
						return types.i32;
					if (e.value.type == TokenType::String)
						return Type{PointerType{std::make_shared<Type>(types.u8)}};
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
					return *left;
				},
				[&](const SizeOfExpr &e) -> std::optional<Type> {
					resolve_type(e.type);
					return types.u64;
				},
				[&](const StructInitExpr &e) -> std::optional<Type> {
					Type resolvedType = resolve_type(e.type);
					if (std::holds_alternative<ErrorType>(resolvedType.kind))
						return std::nullopt;

					std::string structName;
					if (const auto nt = std::get_if<NamedType>(&resolvedType.kind))
						structName = std::string(nt->name_path[0].text);
					else if (const auto st = std::get_if<StructType>(&resolvedType.kind))
						structName = st->name;
					else {
						error(e.l_brace, "Type is not a struct.");
						return std::nullopt;
					}

					const auto defTypeOpt = current_scope_->resolve_type(structName);
					if (!defTypeOpt || !std::holds_alternative<StructType>(defTypeOpt->kind)) {
						error(e.l_brace, "Unknown struct type '" + structName + "'.");
						return std::nullopt;
					}

					const auto &[name, fields] = std::get<StructType>(defTypeOpt->kind);
					for (const auto &[fieldName, fieldValue] : e.fields) {
						auto initType = analyze_expr(*fieldValue);
						if (!initType)
							return std::nullopt;

						bool found = false;
						for (const auto &[defName, defType] : fields) {
							if (defName == fieldName.text) {
								if (!are_types_equal(*initType, defType))
									error(fieldName, "Type mismatch for field '" + std::string(defName) + "'.");
								found = true;
								break;
							}
						}
						if (!found) {
							error(
								fieldName,
								"Struct '" + structName + "' has no field named '" + std::string(fieldName.text) + "'."
							);
						}
					}
					return resolvedType;
				},
				[&](const UnaryExpr &e) -> std::optional<Type> {
					const auto rightType = analyze_expr(*e.right);
					if (!rightType)
						return std::nullopt;

					switch (e.op.type) {
					case TokenType::Minus:
						// todo: Add float check
						if (!is_integer_type(*rightType)) {
							error(e.op, "Operand must be a number.");
							return std::nullopt;
						}
						return *rightType;
					case TokenType::Bang:
						if (!are_types_equal(*rightType, types.bool_)) {
							error(e.op, "Operand must be a boolean.");
							return std::nullopt;
						}
						return types.bool_;
					case TokenType::Ampersand:
						// todo: Verify that e.right is an L-value (variable, field, etc)
						return Type{PointerType{std::make_shared<Type>(*rightType)}};
					case TokenType::Star:
						if (!std::holds_alternative<PointerType>(rightType->kind)) {
							error(e.op, "Cannot dereference non-pointer type.");
							return std::nullopt;
						}
						return *std::get<PointerType>(rightType->kind).inner;
					default:
						return std::nullopt;
					}
				},
				[&](const GroupingExpr &e) -> std::optional<Type> { return analyze_expr(*e.expression); },
				[&](const CallExpr &e) -> std::optional<Type> {
					const auto calleeType = analyze_expr(*e.callee);
					if (!calleeType)
						return std::nullopt;
					if (!std::holds_alternative<FunctionType>(calleeType->kind)) {
						error(e.paren, "Can only call functions and classes.");
						return std::nullopt;
					}
					const auto &[params, return_type, is_variadic, p] = std::get<FunctionType>(calleeType->kind);

					if (is_variadic) {
						if (e.args.size() < params.size()) {
							error(e.paren, "Expected at least " + std::to_string(params.size()) + " arguments.");
							return std::nullopt;
						}
					} else {
						if (e.args.size() != params.size()) {
							error(
								e.paren,
								"Expected " + std::to_string(params.size()) + " arguments but got " +
									std::to_string(e.args.size())
							);
							return std::nullopt;
						}
					}

					for (size_t i = 0; i < params.size(); ++i) {
						auto argType = analyze_expr(e.args[i]);
						if (!argType)
							return std::nullopt;
						if (!are_types_equal(*argType, params[i])) {
							if (!(is_integer_type(*argType) && is_integer_type(params[i])))
								error(e.paren, "Argument " + std::to_string(i + 1) + " type mismatch.");
						}
					}

					for (size_t i = params.size(); i < e.args.size(); ++i) {
						if (!analyze_expr(e.args[i]))
							return std::nullopt;
					}

					if (return_type)
						return *return_type;
					return types.void_;
				},

				[&](const GetExpr &e) -> std::optional<Type> {
					auto objType = analyze_expr(*e.object);
					if (!objType)
						return std::nullopt;

					if (std::holds_alternative<PointerType>(objType->kind))
						objType = *std::get<PointerType>(objType->kind).inner;

					Type structType = *objType;
					if (std::holds_alternative<NamedType>(objType->kind)) {
						const auto &[name_path, generic_args] = std::get<NamedType>(objType->kind);
						if (const auto resolved = current_scope_->resolve_type(std::string(name_path[0].text)))
							structType = *resolved;
					}

					if (!std::holds_alternative<StructType>(structType.kind)) {
						error(e.name, "Only structs have fields.");
						return std::nullopt;
					}

					const auto &[name, fields] = std::get<StructType>(structType.kind);
					for (const auto &[fieldName, fieldType] : fields) {
						if (fieldName == e.name.text)
							return fieldType;
					}

					error(e.name, "Struct '" + name + "' has no field '" + std::string(e.name.text) + "'.");
					return std::nullopt;
				},
				[&](const CastExpr &e) -> std::optional<Type> {
					const auto sourceType = analyze_expr(*e.expr);
					if (!sourceType)
						return std::nullopt;

					Type targetType = resolve_type(e.target_type);

					if (std::holds_alternative<PointerType>(sourceType->kind) &&
						std::holds_alternative<PointerType>(targetType.kind))
						return targetType;

					if (is_integer_type(*sourceType) && is_integer_type(targetType))
						return targetType;

					// todo: Check that targetType is usize/u64
					if (std::holds_alternative<PointerType>(sourceType->kind) && is_integer_type(targetType))
						return targetType;

					if (is_integer_type(*sourceType) && std::holds_alternative<PointerType>(targetType.kind))
						return targetType;

					error(e.expr->get_token(), "Invalid cast operation.");
					return std::nullopt;
				},
				[&](const ArrayAccessExpr &e) -> std::optional<Type> {
					const auto targetType = analyze_expr(*e.target);
					if (!targetType)
						return std::nullopt;

					if (!std::holds_alternative<PointerType>(targetType->kind)) {
						error(e.r_bracket, "Type is not a pointer, cannot index.");
						return std::nullopt;
					}

					const auto indexType = analyze_expr(*e.index);
					if (!indexType)
						return std::nullopt;

					if (!is_integer_type(*indexType)) {
						error(e.index->get_token(), "Array index must be an integer.");
						return std::nullopt;
					}

					return *std::get<PointerType>(targetType->kind).inner;
				},
				[](const auto &) -> std::optional<Type> { return std::nullopt; }
			},
			expr.kind
		);

		if (result)
			expr.type_cache = *result;

		return result;
	}

	bool Semantic::are_types_equal(const Type &a, const Type &b) {
		if (std::holds_alternative<ErrorType>(a.kind) || std::holds_alternative<ErrorType>(b.kind))
			return true;

		if (a.kind.index() != b.kind.index())
			return false;

		if (const auto ba = std::get_if<BuiltinType>(&a.kind))
			return *ba == std::get<BuiltinType>(b.kind);

		if (const auto fa = std::get_if<FunctionType>(&a.kind)) {
			const auto &[params, return_type, is_variadic, p] = std::get<FunctionType>(b.kind);
			if (fa->params.size() != params.size())
				return false;
			for (size_t i = 0; i < fa->params.size(); ++i)
				if (!are_types_equal(fa->params[i], params[i]))
					return false;
			return are_types_equal(*fa->return_type, *return_type);
		}

		if (const auto pa = std::get_if<PointerType>(&a.kind)) {
			const auto [inner] = std::get<PointerType>(b.kind);
			return are_types_equal(*pa->inner, *inner);
		}

		if (const auto sa = std::get_if<SliceType>(&a.kind)) {
			const auto [inner] = std::get<SliceType>(b.kind);
			return are_types_equal(*sa->inner, *inner);
		}

		if (const auto sa = std::get_if<StructType>(&a.kind)) {
			if (const auto sb = std::get_if<StructType>(&b.kind))
				return sa->name == sb->name;
			return false;
		}

		if (const auto na = std::get_if<NamedType>(&a.kind)) {
			const auto [name_path, generic_args] = std::get<NamedType>(b.kind);
			// todo: Simplistic comparison of names for now
			if (na->name_path.size() != name_path.size())
				return false;
			for (size_t i = 0; i < na->name_path.size(); ++i) {
				if (na->name_path[i].text != name_path[i].text)
					return false;
			}
			return true;
		}

		return false;
	}

	bool Semantic::is_integer_type(const Type &type) {
		if (std::holds_alternative<BuiltinType>(type.kind)) {
			const auto builtinType = std::get<BuiltinType>(type.kind);
			return builtinType == BuiltinType::I8 || builtinType == BuiltinType::I16 ||
				   builtinType == BuiltinType::I32 || builtinType == BuiltinType::I64 ||
				   builtinType == BuiltinType::U8 || builtinType == BuiltinType::U16 ||
				   builtinType == BuiltinType::U32 || builtinType == BuiltinType::U64;
		}

		return false;
	}

	bool Semantic::check_int_literal(const std::string &text, const Type &type, const bool is_negative) {
		try {
			const unsigned long long val = std::stoull(text);

			if (std::holds_alternative<BuiltinType>(type.kind)) {
				switch (std::get<BuiltinType>(type.kind)) {
				case BuiltinType::U8:
					return !is_negative && val <= 255;
				case BuiltinType::U16:
					return !is_negative && val <= 65535;
				case BuiltinType::U32:
					return !is_negative && val <= 4294967295ULL;
				case BuiltinType::U64:
					return !is_negative;
				case BuiltinType::I8:
					return (!is_negative && val <= 127) || (is_negative && val <= 128);
				case BuiltinType::I16:
					return (!is_negative && val <= 32767) || (is_negative && val <= 32768);
				case BuiltinType::I32:
					return (!is_negative && val <= 2147483647ULL) || (is_negative && val <= 2147483648ULL);
				case BuiltinType::I64:
					return is_negative ? (val <= 9223372036854775808ULL) : (val <= 9223372036854775807ULL);
				default:
					return false;
				}
			}

			return false;
		} catch (...) {
			return false;
		}
	}
} // namespace hadron::frontend
