#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "types.hpp"

namespace hadron::frontend {
	struct Symbol {
		std::string name;
		Type type;
	};

	struct TypeSymbol {
		std::string name;
		Type type;
	};

	class Scope {
	public:
		using Ptr = std::shared_ptr<Scope>;

		explicit Scope(Ptr parent = nullptr) : parent_(std::move(parent)) {}

		bool define_value(const std::string &name, const Type &type) {
			if (values_.contains(name))
				return false;
			values_[name] = Symbol{name, type};
			return true;
		}

		[[nodiscard]] std::optional<Symbol> resolve_value(const std::string &name) const {
			if (const auto it = values_.find(name); it != values_.end())
				return it->second;
			return parent_ ? parent_->resolve_value(name) : std::nullopt;
		}

		bool define_type(const std::string &name, const Type &type) {
			return types_.emplace(name, TypeSymbol{name, type}).second;
		}

		void redefine_type(const std::string &name, const Type &type) {
			types_[name] = TypeSymbol{name, type};
		}

		[[nodiscard]] std::optional<Type> resolve_type(const std::string &name) const {
			if (auto it = types_.find(name); it != types_.end())
				return it->second.type;
			return parent_ ? parent_->resolve_type(name) : std::nullopt;
		}

		[[nodiscard]] Ptr parent() const {
			return parent_;
		}

	private:
		std::unordered_map<std::string, Symbol> values_;
		std::unordered_map<std::string, TypeSymbol> types_;
		Ptr parent_;
	};
} // namespace hadron::frontend
