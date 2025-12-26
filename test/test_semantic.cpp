#include <gtest/gtest.h>

#include "frontend/semantic.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"

using namespace hadron::frontend;

bool analyze_source(std::string_view source) {
	Lexer lexer(source);
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));
	auto unit = parser.parse();
	Semantic sem(unit);
	return sem.analyze();
}

TEST(SemanticTest, HandleModules) {
	const bool result = analyze_source(
		"module my.app");
	EXPECT_TRUE(result);
}