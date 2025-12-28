#include "frontend/ast.hpp"

namespace hadron::frontend {
	template <class... Ts> struct overloaded : Ts... {
		using Ts::operator()...;
	};
	template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	const Token &Expr::get_token() const {
		return std::visit(
			overloaded{
				[](const LiteralExpr &e) -> const Token & { return e.value; },
				[](const VariableExpr &e) -> const Token & { return e.name; },
				[](const BinaryExpr &e) -> const Token & { return e.op; },
				[](const UnaryExpr &e) -> const Token & { return e.op; },
				[](const GroupingExpr &e) -> const Token & { return e.paren; },
				[](const CallExpr &e) -> const Token & { return e.paren; },
				[](const CastExpr &e) -> const Token & { return e.op; },
				[](const GetExpr &e) -> const Token & { return e.name; },
				[](const SizeOfExpr &e) -> const Token & { return e.keyword; },
				[](const StructInitExpr &e) -> const Token & { return e.l_brace; },
				[](const ArrayAccessExpr &e) -> const Token & { return e.r_bracket; },
				[](const ElseExpr &e) -> const Token & { return e.keyword; }
			},
			kind
		);
	}
} // namespace hadron::frontend
