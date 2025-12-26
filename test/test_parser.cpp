#include <gtest/gtest.h>

#include "frontend/ast.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"

using namespace hadron::frontend;

template <typename T, typename Variant> bool is_type(const Variant &variant) {
	return std::holds_alternative<T>(variant.kind);
}

template <typename T, typename Variant> const T &get_node(const Variant &variant) {
	return std::get<T>(variant.kind);
}

CompilationUnit parse_source(std::string_view source) {
	Lexer lexer(source);
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));
	return parser.parse();
}

const Stmt &get_first_stmt_of_main(const CompilationUnit &unit) {
	const auto &funcDecl = get_node<FunctionDecl>(unit.declarations[0]);
	return funcDecl.body[0];
}

TEST(ParserTest, HndlesModuleAndImports) {
	const auto unit = parse_source(
		"module my.app;"
		"import std.io;"
		"import math as m;"
	);

	EXPECT_EQ(unit.module.name_path.size(), 2);
	EXPECT_EQ(unit.module.name_path[0].text, "my");
	EXPECT_EQ(unit.module.name_path[1].text, "app");

	ASSERT_EQ(unit.imports.size(), 2);

	EXPECT_EQ(unit.imports[0].path[0].text, "std");
	EXPECT_EQ(unit.imports[0].path[1].text, "io");
	EXPECT_FALSE(unit.imports[0].alias.has_value());

	EXPECT_EQ(unit.imports[1].path[0].text, "math");
	EXPECT_TRUE(unit.imports[1].alias.has_value());
	EXPECT_EQ(unit.imports[1].alias->text, "m");
}

TEST(ParserTest, RejectsTopLevelStatements) {
	const CompilationUnit unit = parse_source("module test; var x = 1;");
	EXPECT_EQ(unit.declarations.size(), 0);
}

TEST(ParserTest, HandlesSimpleExpression) {
	const CompilationUnit unit = parse_source("module test; fx main() { 1 + 2; }");
	const auto &stmt = get_first_stmt_of_main(unit);
	const auto &[expression] = get_node<ExpressionStmt>(stmt);
	const auto &[left, op, right] = get_node<BinaryExpr>(expression);
	EXPECT_EQ(op.type, TokenType::Plus);
}

TEST(ParserTest, HandlesFunctionDeclaration) {
	const CompilationUnit unit = parse_source("module test; fx add(a: i32, b: i32) i32 { return a + b; }");
	ASSERT_EQ(unit.declarations.size(), 1);

	const auto &[name, params, retType, body] = get_node<FunctionDecl>(unit.declarations[0]);

	EXPECT_EQ(name.text, "add");
	EXPECT_EQ(params.size(), 2);
	EXPECT_EQ(params[0].name.text, "a");

	ASSERT_TRUE(retType.has_value());
	const auto &rType = get_node<NamedType>(*retType);
	EXPECT_EQ(rType.name_path[0].text, "i32");
}

TEST(ParserTest, HandlesVoidFunction) {
	const CompilationUnit unit = parse_source("module test; fx log() { }");
	const auto &[name, params, retType, body] = get_node<FunctionDecl>(unit.declarations[0]);
	EXPECT_FALSE(retType.has_value());
}

TEST(ParserTest, HandlesIfElse) {
	const CompilationUnit unit = parse_source("module test; fx main() { if (true) { } else { } }");
	const auto &stmt = get_first_stmt_of_main(unit);

	ASSERT_TRUE(is_type<IfStmt>(stmt));
	const auto &[cond, thenB, elseB] = get_node<IfStmt>(stmt);
	ASSERT_TRUE(thenB != nullptr);
	ASSERT_TRUE(elseB != nullptr);
}

TEST(ParserTest, HandlesVarDeclaration) {
	const CompilationUnit unit = parse_source("module test; fx main() { val x: i32 = 10; }");
	const auto &stmt = get_first_stmt_of_main(unit);

	ASSERT_TRUE(is_type<VarDeclStmt>(stmt));
	const auto &[name, init, type, is_mut, _p] = get_node<VarDeclStmt>(stmt);
	EXPECT_EQ(name.text, "x");
	EXPECT_TRUE(type.has_value());
}

TEST(ParserTest, HandlesStructDeclaration) {
	const auto unit = parse_source(
		"module game;"
		"struct Point {"
		"	x: i32;"
		"	y: i32;"
		"}"
	);

	ASSERT_EQ(unit.declarations.size(), 1);

	ASSERT_TRUE(is_type<StructDecl>(unit.declarations[0]));
	const auto &[name, fields] = get_node<StructDecl>(unit.declarations[0]);

	EXPECT_EQ(fields[0].name.text, "x");
	ASSERT_TRUE(is_type<NamedType>(fields[0].type));
	EXPECT_EQ(get_node<NamedType>(fields[0].type).name_path[0].text, "i32");

	EXPECT_EQ(fields[1].name.text, "y");
	ASSERT_TRUE(is_type<NamedType>(fields[1].type));
	EXPECT_EQ(get_node<NamedType>(fields[1].type).name_path[0].text, "i32");
}

TEST(ParserTest, HandlesComplexStruct) {
	auto unit = parse_source(
		"module list;"
		"struct Node {"
		"	next: ptr<Node>;"
		"	data: slice<u8>;"
		"}"
	);

	ASSERT_EQ(unit.declarations.size(), 1);
	const auto &[name, fields] = get_node<StructDecl>(unit.declarations[0]);

	EXPECT_EQ(fields[0].name.text, "next");
	ASSERT_TRUE(is_type<PointerType>(fields[0].type));

	EXPECT_EQ(fields[1].name.text, "data");
	ASSERT_TRUE(is_type<SliceType>(fields[1].type));
}
