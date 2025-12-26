#include <print>

#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "frontend/lexer.hpp"

int main(void) {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	std::print("Initialising Hadron Compiler...\n");
	llvm::outs() << "LLVM Backend active. Target architecture detected.\n";
	return 0;
}
