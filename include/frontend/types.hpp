#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "token.hpp"

namespace hadron::frontend {

	struct Type;

	enum class BuiltinType {
		Byte,
		I8,
		I16,
		I32,
		I64,
		U8,
		U16,
		U32,
		U64,
		F32,
		F64,
		Bool,
		Void,
	};

	struct ErrorType {};

	struct NamedType {
		std::vector<Token> name_path;
		std::vector<Type> generic_args;
	};

	struct FunctionType {
		std::vector<Type> params;
		std::shared_ptr<Type> return_type;
		bool is_variadic;
		char padding[7];
	};

	struct PointerType {
		std::shared_ptr<Type> inner;
	};

	struct SliceType {
		std::shared_ptr<Type> inner;
	};

	struct StructType {
		std::string name;
		std::vector<std::pair<std::string, Type>> fields;
	};

	struct Type {
		using Kind = std::variant<BuiltinType, NamedType, PointerType, SliceType, FunctionType, ErrorType, StructType>;
		Kind kind;
	};

	struct TypeTable {
		Type byte{BuiltinType::Byte};
		Type i8{BuiltinType::I8};
		Type i16{BuiltinType::I16};
		Type i32{BuiltinType::I32};
		Type i64{BuiltinType::I64};
		Type u8{BuiltinType::U8};
		Type u16{BuiltinType::U16};
		Type u32{BuiltinType::U32};
		Type u64{BuiltinType::U64};
		Type f32{BuiltinType::F32};
		Type f64{BuiltinType::F64};
		Type bool_{BuiltinType::Bool};
		Type void_{BuiltinType::Void};
		Type error{ErrorType{}};
	};

} // namespace hadron::frontend
