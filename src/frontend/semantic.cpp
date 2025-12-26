#include "frontend/semantic.hpp"

#include <variant>

namespace hadron::frontend {
	Semantic::Semantic(const CompilationUnit &unit) : unit_(unit) {}

	std::vector<SemanticError> Semantic::analyze() {
		for (auto &decl : unit_.declarations)
			visit(decl);
		return errors_;
	}

	void Semantic::visit(const Stmt &stmt) {
	    std::visit([this](auto &&s) {
		    this->visit(s);
	    }, stmt.kind);
	}

	void Semantic::visit(const BlockStmt &block) {
		Scope local;
		local.parent = currentScope_;
		currentScope_ = &local;

		for (const Stmt &stmt : block.statements)
			visit(stmt);

		currentScope_ = local.parent;
	}

	void Semantic::visit(const VarDeclStmt &var) {
		if (currentScope_->symbols.contains(var.name.to_string())) {
			errors_.emplace_back("Variable already declared");
			return;
		}
		if (var.initializer)
			visit(*var.initializer);
		const Symbol symbol(var.name.text);
		currentScope_->symbols[var.name.to_string()] = symbol;
	}

	void Semantic::visit(const IfStmt &stmt) {
		const auto &cond = stmt.condition;
		const auto &thenB = stmt.then_branch;
		const auto &elseB = stmt.else_branch;
		visit(cond);
		if (thenB)
			visit(*thenB);
		if (elseB)
			visit(*elseB);
	}

	void Semantic::visit(const WhileStmt &stmt) {
		const auto &cond = stmt.condition;
		const auto &body = stmt.body;
		visit(cond);
		if (body)
			visit(*body);
	}

	void Semantic::visit(const ExpressionStmt &expr) {
		visit(expr.expression);
	}

	void Semantic::visit(const ReturnStmt &ret) {
		if (ret.value)
			visit(*ret.value);
	}

	void Semantic::visit(const FunctionDecl &fx) {
		Scope local;
		local.parent = currentScope_;
		currentScope_ = &local;

		for (const auto &param : fx.params)
			visit(param);

		for (const auto &stmt : fx.body)
			visit(stmt);
		currentScope_ = local.parent;

		const Symbol symbol(fx.name.text);
		currentScope_->symbols[fx.name.to_string()] = symbol;
	}

	void Semantic::visit(const Expr &expr) {
		std::visit([this](auto &&s) {
			this->visit(s);
		}, expr.kind);
	}

	void Semantic::visit(const BinaryExpr &expr) {
		visit(*expr.left);
		visit(*expr.right);
	}

	void Semantic::visit(const Param &param) const {
		const Symbol symbol(param.name.text);
		currentScope_->symbols[param.name.to_string()] = symbol;
	}

	void Semantic::visit(const GroupingExpr &group) {
		if (group.expression)
			visit(*group.expression);
	}

	void Semantic::visit(const LiteralExpr &) {}

	void Semantic::visit(const UnaryExpr &expr) {
		if (expr.right)
			visit(*expr.right);
	}

	void Semantic::visit(const VariableExpr &expr) {
		if (!currentScope_->symbols.contains(expr.name.to_string())) {
			errors_.emplace_back("Variable not declared");
		}
	}
} // namespace hadron::frontend