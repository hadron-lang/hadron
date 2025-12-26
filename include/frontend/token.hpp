#pragma once

#include <string_view>

#include "hadron.hpp"

namespace hadron::frontend {
	enum class TokenType : u8 {
		Eof,
		Error,

		Identifier,
		Number,
		String,

		KwValue,
		KwStruct,
		KwClass,
		KwInterface,
		KwEnum,
		KwUnion,
		KwIf,
		KwElse,
		KwFor,
		KwWhile,
		KwLoop,
		KwBreak,
		KwContinue,
		KwReturn,
		KwFx,
		KwStatic,
		KwSpawn,
		KwThread,
		KwAsync,
		KwAwait,
		KwVar,
		KwVal,
		KwNull,
		KwTrue,
		KwFalse,
		KwModule,
		KwImport,
		KwAs,
		KwPub,
		KwPriv,
		KwTry,
		KwCatch,
		KwThrow,

		Plus,
		Minus,
		Star,
		Slash,
		Percent,
		Eq,
		EqEq,
		Bang,
		BangEq,
		Lt,
		LtEq,
		Gt,
		GtEq,
		Arrow,
		FatArrow,
		Ampersand,
		Pipe,
		Caret,

		LParen,
		RParen,
		LBrace,
		RBrace,
		LBracket,
		RBracket,
		Comma,
		Dot,
		Colon,
		Semicolon
	};

	struct SourceLocation {
		u32 line;
		u32 column;
		usize offset;
	};

	struct Token {
		std::string_view text;
		SourceLocation location;
		TokenType type;
		char padding[7];

		[[nodiscard]] std::string to_string() const;
	};

	[[nodiscard]] std::string_view token_type_to_string(TokenType type);
} // namespace hadron::frontend
