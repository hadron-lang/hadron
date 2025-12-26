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

TEST(ParserTest, HandlesSimpleExpression) {
	Lexer lexer("1 + 2;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	ASSERT_TRUE(is_type<ExpressionStmt>(statements[0]));
	const auto &[expression] = get_node<ExpressionStmt>(statements[0]);

	ASSERT_TRUE(is_type<BinaryExpr>(expression));
	const auto &[left, op, right] = get_node<BinaryExpr>(expression);
	EXPECT_EQ(op.type, TokenType::Plus);
}

TEST(ParserTest, HandlesPrecedence) {
	Lexer lexer("1 + 2 * 3;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	const auto &[expression] = get_node<ExpressionStmt>(statements[0]);
	const auto &[left, op, right] = get_node<BinaryExpr>(expression);
	EXPECT_EQ(op.type, TokenType::Plus);

	ASSERT_TRUE(is_type<LiteralExpr>(*left));
	const auto &[leftVal] = get_node<LiteralExpr>(*left);
	EXPECT_EQ(leftVal.text, "1");

	ASSERT_TRUE(is_type<BinaryExpr>(*right));
	const auto &[rLeft, rOp, rRight] = get_node<BinaryExpr>(*right);
	EXPECT_EQ(rOp.type, TokenType::Star);
}

TEST(ParserTest, HandlesGrouping) {
	Lexer lexer("(1 + 2) * 3;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	const auto &[expression] = get_node<ExpressionStmt>(statements[0]);

	ASSERT_TRUE(is_type<BinaryExpr>(expression));
	const auto &[left, op, right] = get_node<BinaryExpr>(expression);
	EXPECT_EQ(op.type, TokenType::Star);

	ASSERT_TRUE(is_type<GroupingExpr>(*left));
	const auto &[innerExpr] = get_node<GroupingExpr>(*left);

	ASSERT_TRUE(is_type<BinaryExpr>(*innerExpr));
	const auto &[gLeft, gOp, gRight] = get_node<BinaryExpr>(*innerExpr);
	EXPECT_EQ(gOp.type, TokenType::Plus);

	ASSERT_TRUE(is_type<LiteralExpr>(*right));
	const auto &[rightVal] = get_node<LiteralExpr>(*right);
	EXPECT_EQ(rightVal.text, "3");
}

TEST(ParserTest, HandlesUnaryExpression) {
	Lexer lexer("-123; !true;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 2);

	{
		const auto &[expr] = get_node<ExpressionStmt>(statements[0]);
		ASSERT_TRUE(is_type<UnaryExpr>(expr));

		const auto &[op, right] = get_node<UnaryExpr>(expr);
		EXPECT_EQ(op.type, TokenType::Minus);

		ASSERT_TRUE(is_type<LiteralExpr>(*right));
		const auto &[val] = get_node<LiteralExpr>(*right);
		EXPECT_EQ(val.text, "123");
	}

	{
		const auto &[expr] = get_node<ExpressionStmt>(statements[1]);
		ASSERT_TRUE(is_type<UnaryExpr>(expr));

		const auto &[op, right] = get_node<UnaryExpr>(expr);
		EXPECT_EQ(op.type, TokenType::Bang);

		ASSERT_TRUE(is_type<LiteralExpr>(*right));
		const auto &[val] = get_node<LiteralExpr>(*right);
		EXPECT_EQ(val.text, "true");
	}
}

TEST(ParserTest, HandlesComparisons) {
	Lexer lexer("x == y; a > b;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));
	const auto statements = parser.parse();

	const auto &[expr1] = get_node<ExpressionStmt>(statements[0]);
	const auto &[lhs1, op1, rhs1] = get_node<BinaryExpr>(expr1);
	EXPECT_EQ(op1.type, TokenType::EqEq);

	const auto &[expr2] = get_node<ExpressionStmt>(statements[1]);
	const auto &[lhs2, op2, rhs2] = get_node<BinaryExpr>(expr2);
	EXPECT_EQ(op2.type, TokenType::Gt);
}

TEST(ParserTest, HandlesVarDeclaration) {
	Lexer lexer("val x = 10; var y = 0;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 2);

	ASSERT_TRUE(is_type<VarDeclStmt>(statements[0]));
	const auto &[name1, init1, _t, mut1, _p] = get_node<VarDeclStmt>(statements[0]);
	EXPECT_EQ(name1.text, "x");
	EXPECT_FALSE(mut1);
	ASSERT_TRUE(init1 != nullptr);

	ASSERT_TRUE(is_type<VarDeclStmt>(statements[1]));
	const auto &[name2, init2, _t2, mut2, _p2] = get_node<VarDeclStmt>(statements[1]);
	EXPECT_EQ(name2.text, "y");
	EXPECT_TRUE(mut2);
}

TEST(ParserTest, HandlesBlocks) {
	Lexer lexer("{ val x = 1; { 2; } }");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	ASSERT_TRUE(is_type<BlockStmt>(statements[0]));
	const auto &[blockStmts] = get_node<BlockStmt>(statements[0]);
	ASSERT_EQ(blockStmts.size(), 2);

	ASSERT_TRUE(is_type<VarDeclStmt>(blockStmts[0]));

	ASSERT_TRUE(is_type<BlockStmt>(blockStmts[1]));
	const auto &[innerStmts] = get_node<BlockStmt>(blockStmts[1]);

	ASSERT_EQ(innerStmts.size(), 1);
	ASSERT_TRUE(is_type<ExpressionStmt>(innerStmts[0]));
}

TEST(ParserTest, HandlesIfStatement) {
	Lexer lexer("if (x) { val y = 1; }");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	ASSERT_TRUE(is_type<IfStmt>(statements[0]));
	const auto &[cond, thenBranch, elseBranch] = get_node<IfStmt>(statements[0]);

	ASSERT_TRUE(is_type<VariableExpr>(cond));
	EXPECT_EQ(get_node<VariableExpr>(cond).name.text, "x");

	ASSERT_TRUE(thenBranch != nullptr);
	ASSERT_TRUE(is_type<BlockStmt>(*thenBranch));
	const auto &[blockStmts] = get_node<BlockStmt>(*thenBranch);
	ASSERT_EQ(blockStmts.size(), 1);

	ASSERT_TRUE(elseBranch == nullptr);
}

TEST(ParserTest, HandlesIfElseStatement) {
	Lexer lexer("if (true) { } else { }");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	const auto &[cond, thenBranch, elseBranch] = get_node<IfStmt>(statements[0]);

	ASSERT_TRUE(thenBranch != nullptr);
	ASSERT_TRUE(elseBranch != nullptr);

	ASSERT_TRUE(is_type<BlockStmt>(*thenBranch));
	ASSERT_TRUE(is_type<BlockStmt>(*elseBranch));
}

TEST(ParserTest, HandlesElseIfRecursion) {
	Lexer lexer("if (a) { } else if (b) { }");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	const auto &[cond, thenBranch, elseBranch] = get_node<IfStmt>(statements[0]);

	ASSERT_TRUE(elseBranch != nullptr);
	ASSERT_TRUE(is_type<IfStmt>(*elseBranch));

	const auto &nestedIf = get_node<IfStmt>(*elseBranch);
	ASSERT_TRUE(is_type<VariableExpr>(nestedIf.condition));
	EXPECT_EQ(get_node<VariableExpr>(nestedIf.condition).name.text, "b");
}

TEST(ParserTest, HandlesWhileStatement) {
	Lexer lexer("while (running) { val a = 1; }");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	ASSERT_TRUE(is_type<WhileStmt>(statements[0]));
	const auto &[cond, body] = get_node<WhileStmt>(statements[0]);

	ASSERT_TRUE(is_type<VariableExpr>(cond));
	EXPECT_EQ(get_node<VariableExpr>(cond).name.text, "running");

	ASSERT_TRUE(body != nullptr);
	ASSERT_TRUE(is_type<BlockStmt>(*body));
}

TEST(ParserTest, HandlesTypedVarDeclaration) {
	Lexer lexer("var x: i32 = 10;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);

	const auto &[name, init, type, is_mut, _p1] = get_node<VarDeclStmt>(statements[0]);
	EXPECT_EQ(name.text, "x");
	EXPECT_TRUE(type.has_value());

	ASSERT_TRUE(is_type<NamedType>(*type));
	const auto &[name_path, generic_args] = get_node<NamedType>(*type);
	ASSERT_EQ(name_path.size(), 1);
	EXPECT_EQ(name_path[0].text, "i32");
}

TEST(ParserTest, HandlesComplexTypes) {
	Lexer lexer("val x: ptr<slice<std.Map<String, i32>>> = null;");
	auto tokens = lexer.tokenize();
	Parser parser(std::move(tokens));

	const auto statements = parser.parse();
	ASSERT_EQ(statements.size(), 1);
	const auto &[name, init, type, is_mut, _p1] = get_node<VarDeclStmt>(statements[0]);

	ASSERT_TRUE(type.has_value());

	ASSERT_TRUE(is_type<PointerType>(*type));
	const auto &[inner1] = get_node<PointerType>(*type);
	ASSERT_TRUE(is_type<SliceType>(*inner1));
	const auto &[inner2] = get_node<SliceType>(*inner1);
	ASSERT_TRUE(is_type<NamedType>(*inner2));

	const auto &[name_path, generic_args] = get_node<NamedType>(*inner2);

	ASSERT_EQ(name_path.size(), 2);
	EXPECT_EQ(name_path[0].text, "std");
	EXPECT_EQ(name_path[1].text, "Map");

	ASSERT_EQ(generic_args.size(), 2);

	const auto &[name_path2, generic_args2] = get_node<NamedType>(generic_args[0]);
	EXPECT_EQ(name_path2[0].text, "String");

	const auto &[name_path3, generic_args3] = get_node<NamedType>(generic_args[1]);
	EXPECT_EQ(name_path3[0].text, "i32");
}
