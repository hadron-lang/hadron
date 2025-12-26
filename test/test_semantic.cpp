#include <gtest/gtest.h>

#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/semantic.hpp"

using namespace hadron::frontend;

Semantic get_semantic_analysis(std::string_view source) {
	Lexer lexer(source);
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));
	auto unit = parser.parse();
	Semantic sem(unit);
	(void)sem.analyze();
	return sem;
}

bool has_error(const std::vector<std::string> &errors, const std::string &partial) {
	for (const auto &err : errors) {
		if (err.find(partial) != std::string::npos)
			return true;
	}
	return false;
}

TEST(SemanticTest, DetectsUndefinedVariable) {
	const auto sem = get_semantic_analysis("module my.app; fx main() i32 { return x; }");
	ASSERT_FALSE(sem.errors().empty());
	EXPECT_TRUE(sem.errors()[0].find("Undefined variable 'x'") != std::string::npos);
}

TEST(SemanticTest, HandlesVariableDeclaration) {
	const auto sem = get_semantic_analysis("module my.app; fx main() i32 { val x = 1; return x; }");
	EXPECT_TRUE(sem.errors().empty());
}

TEST(SemanticTest, DetectsShadowingOrRedefinition) {
	const auto sem = get_semantic_analysis("module my.app; fx main() i32 { val x = 1; val x = 2; return x; }");
	ASSERT_FALSE(sem.errors().empty());
	EXPECT_TRUE(sem.errors()[0].find("already declared") != std::string::npos);
}

TEST(SemanticTest, HandlesBlockScopes) {
	const auto sem = get_semantic_analysis(
		"module my.app;"
		"fx main() i32 {"
		"	val x = 1;"
		"	{"
		"		val x = 2;"
		"		return x;"
		"	}"
		"	return x;"
		"}"
	);
	EXPECT_TRUE(sem.errors().empty());
}

TEST(SemanticTest, HandlesScopeInheritance) {
	const auto sem = get_semantic_analysis(
		"module my.app;"
		"fx main() i32 {"
		"	val x = 1;"
		"	{"
		"		return x;"
		"	}"
		"	return x;"
		"}"
	);
	EXPECT_TRUE(sem.errors().empty());
}

TEST(SemanticTest, DetectsReturnValueInVoidFunction) {
	const auto sem = get_semantic_analysis("module test; fx main() { return 1; }");
	ASSERT_FALSE(sem.errors().empty());
	EXPECT_TRUE(has_error(sem.errors(), "Void function should not return a value"));
}

TEST(SemanticTest, DetectsMissingReturnValue) {
	const auto sem = get_semantic_analysis("module test; fx add() i32 { return; }");
	ASSERT_FALSE(sem.errors().empty());
	EXPECT_TRUE(has_error(sem.errors(), "Non-void function should return a value"));
}
