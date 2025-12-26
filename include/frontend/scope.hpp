#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace hadron::frontend {
	struct Symbol {
		std::string name;
		std::string type;
	};

	class Scope {
	public:
		using Ptr = std::shared_ptr<Scope>;

		explicit Scope(Ptr parent = nullptr) : parent_(std::move(parent)) {}

		bool define(const std::string &name, const std::string &type) {
			if (symbols_.contains(name))
				return false;
			symbols_[name] = Symbol{name, type};
			return true;
		}

		[[nodiscard]] std::optional<Symbol> resolve(const std::string &name) const {
			if (const auto it = symbols_.find(name); it != symbols_.end())
				return it->second;
			if (parent_)
				return parent_->resolve(name);
			return std::nullopt;
		}

		[[nodiscard]] Ptr parent() const {
			return parent_;
		}

	private:
		std::unordered_map<std::string, Symbol> symbols_;
		Ptr parent_;
	};
} // namespace hadron::frontend
