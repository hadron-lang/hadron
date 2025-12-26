#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace hadron::frontend {
	struct Symbol {
		std::string_view name;
		std::string_view type;
	};

	struct Scope {
		std::unordered_map<std::string, Symbol> symbols;
		std::shared_ptr<Scope> parent;
	};
}