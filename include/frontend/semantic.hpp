#pragma once

#include "ast.hpp"

namespace hadron::frontend {
	class Semantic {
		CompilationUnit &unit_;

	public:
		explicit Semantic(CompilationUnit &unit);

		[[nodiscard]] bool analyze();
	};
}
