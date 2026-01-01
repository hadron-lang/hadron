#pragma once

#include "semantic.hpp"

namespace hadron::frontend {
	inline std::string get_type_name(const Type &t) {
		if (const auto st = std::get_if<StructType>(&t.kind))
			return st->name;
		if (const auto nt = std::get_if<NamedType>(&t.kind))
			return std::string(nt->name_path[0].text);
		return "";
	}
} // namespace hadron::frontend
