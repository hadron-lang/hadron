#include <iostream>
#include <print>

#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "backend/codegen.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/semantic.hpp"

constexpr std::string_view TEST_SOURCE = R"(
module main;

fx main() i32 {
	var sum = 0;
	var i = 0;
	while (i < 10) {
		sum = sum + i;
		i = i + 1;
	}
	return sum;
}
)";

int main(void) {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	std::print("Initialising Hadron Compiler...\n");

	hadron::frontend::Lexer lexer(TEST_SOURCE);
	auto tokens = lexer.tokenize();
	hadron::frontend::Parser parser(std::move(tokens));
	auto unit = parser.parse();

	hadron::frontend::Semantic semantic(unit);
	if (!semantic.analyze()) {
		std::print(stderr, "Semantic errors:\n");
		for (const auto &err : semantic.errors()) {
			std::cerr << "	" << err << "\n";
		}
		return 1;
	}

	hadron::backend::CodeGenerator code_generator(unit);
	code_generator.generate();
	code_generator.emit_object("output.o");

	return 0;
}
