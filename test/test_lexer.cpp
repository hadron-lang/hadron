#include <gtest/gtest.h>
#include <vector>

#include "frontend/lexer.hpp"

using namespace hadron::frontend;

TEST(LexerTest, HandlesEmptyInput) {
	Lexer lexer("");
	const auto tokens = lexer.tokenize();
	ASSERT_EQ(tokens.size(), 1);
	EXPECT_EQ(tokens[0].type, TokenType::Eof);
}

TEST(LexerTest, HandlesKeywordsAndIdentifiers) {
	Lexer lexer("val x = true; fx main");
	const auto tokens = lexer.tokenize();
	ASSERT_EQ(tokens.size(), 8);
	EXPECT_EQ(tokens[0].type, TokenType::KwVal);
	EXPECT_EQ(tokens[1].type, TokenType::Identifier);
	EXPECT_EQ(tokens[1].text, "x");
	EXPECT_EQ(tokens[5].type, TokenType::KwFx);
	EXPECT_EQ(tokens[5].text, "fx");
	EXPECT_EQ(tokens[7].type, TokenType::Eof);
}

TEST(LexerTest, HandlesNumbers) {
	Lexer lexer("123 45.67");
	const auto tokens = lexer.tokenize();
	EXPECT_EQ(tokens[0].type, TokenType::Number);
	EXPECT_EQ(tokens[0].text, "123");
	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[1].text, "45.67");
}

TEST(LexerTest, HandlesComments) {
	Lexer lexer("val x // this is ignored\n = 1;");
	const auto tokens = lexer.tokenize();
	ASSERT_EQ(tokens.size(), 6);
	EXPECT_EQ(tokens[0].type, TokenType::KwVal);
	EXPECT_EQ(tokens[1].type, TokenType::Identifier);
	EXPECT_EQ(tokens[2].type, TokenType::Eq);
	EXPECT_EQ(tokens[3].type, TokenType::Number);
}

TEST(LexerTest, HandlesComplexOperators) {
	Lexer lexer("= == => - ->");
	const auto tokens = lexer.tokenize();
	ASSERT_EQ(tokens.size(), 6);
	EXPECT_EQ(tokens[0].type, TokenType::Eq);
	EXPECT_EQ(tokens[1].type, TokenType::EqEq);
	EXPECT_EQ(tokens[2].type, TokenType::FatArrow);
	EXPECT_EQ(tokens[3].type, TokenType::Minus);
	EXPECT_EQ(tokens[4].type, TokenType::Arrow);
}

TEST(LexerTest, TracksSourceLocation) {
	Lexer lexer("a\n  b");
	const auto tokens = lexer.tokenize();
	EXPECT_EQ(tokens[0].text, "a");
	EXPECT_EQ(tokens[0].location.line, 1);
	EXPECT_EQ(tokens[0].location.column, 1);
	EXPECT_EQ(tokens[1].text, "b");
	EXPECT_EQ(tokens[1].location.line, 2);
	EXPECT_EQ(tokens[1].location.column, 3);
}

TEST(LexerTest, DetectsUnterminatedString) {
	Lexer lexer("val s = \"oops");
	const auto tokens = lexer.tokenize();
	EXPECT_EQ(tokens.back().type, TokenType::Eof);
	const auto &errorToken = tokens[tokens.size() - 2];
	EXPECT_EQ(errorToken.type, TokenType::Error);
	EXPECT_EQ(errorToken.text, "Unterminated String literal");
}

TEST(LexerTest, DetectsUnknownCharacters) {
	Lexer lexer("val $");
	const auto tokens = lexer.tokenize();
	EXPECT_EQ(tokens[1].type, TokenType::Error);
	EXPECT_EQ(tokens[1].text, "Unexpected character");
}
