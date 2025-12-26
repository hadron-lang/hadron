#include <gtest/gtest.h>

#include "frontend/semantic.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"

using namespace hadron::frontend;

auto analyze_source(const std::string_view source) {
	Lexer lexer(source);
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));
	const auto &unit = parser.parse();
	Semantic sem(unit);
	return sem.analyze();
}

TEST(SemanticTest, HandleModules) {
	const auto result = analyze_source(
		"module my.app;");
	EXPECT_EQ(result.size(), 0);
}

TEST(SemanticTest, CompleteTest) {
	const auto result = analyze_source(
		"module my.app;"
		"fx main() {"
		"	a + b;"
		"}");
	for (const auto &err : result) {
		std::cout << err.what();
	}
}