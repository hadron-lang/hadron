#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace hadron::frontend {
	struct Symbol {
		std::string_view name;
		std::string_view type;
	};

	class SemanticError : public std::runtime_error {
	public:
		explicit SemanticError(std::string const& message) : runtime_error(message) {}
	};

	struct Scope {
		std::unordered_map<std::string, Symbol> symbols{};
		Scope *parent{nullptr};
	};
}