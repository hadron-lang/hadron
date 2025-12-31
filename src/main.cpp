#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <vector>

#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "backend/codegen.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/semantic.hpp"

namespace fs = std::filesystem;

struct CompilerConfig {
	std::string input_file;
	std::string output_file = "a.out";
	bool emit_ir = false;
	bool dump_asm = false;
	bool keep_obj = false;
};

static std::string read_file(const std::string &path) {
	std::ifstream file(path, std::ios::binary);
	if (!file)
		throw std::runtime_error("Could not open file: " + path);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return content;
}

static CompilerConfig parse_args(const int argc, char **argv) {
	CompilerConfig config;

	if (argc < 2) {
		std::print(stderr, "Usage: hadronc <input.hd> [options]\n");
		std::print(stderr, "Options:\n");
		std::print(stderr, "  -o <file>    Output binary name\n");
		std::print(stderr, "  --emit-ir    Print LLVM IR to stdout\n");
		std::print(stderr, "  --dump-asm   Display assembly code (objdump)\n");
		std::print(stderr, "  --keep-obj   Keep the intermediate .o file\n");
		exit(1);
	}

	for (int i = 1; i < argc; ++i) {
		if (std::string arg = argv[i]; arg == "-o") {
			if (i + 1 < argc)
				config.output_file = argv[++i];
			else
				throw std::runtime_error("Missing filename after -o");
		} else if (arg == "--emit-ir") {
			config.emit_ir = true;
		} else if (arg == "--dump-asm") {
			config.dump_asm = true;
		} else if (arg == "--keep-obj") {
			config.keep_obj = true;
		} else {
			if (config.input_file.empty())
				config.input_file = arg;
			else
				throw std::runtime_error("Unknown argument: " + arg);
		}
	}

	if (config.input_file.empty())
		throw std::runtime_error("No input file specified.");

	return config;
}

void run_linker(const std::string &obj_file, const std::string &out_file) {
	std::string cmd = "clang -fuse-ld=lld " + obj_file + " -o " + out_file;

	if (const int ret = std::system(cmd.c_str()); ret != 0) {
		std::cerr << "Warning: linking with lld failed, retrying with default linker...\n";
		cmd = "clang " + obj_file + " -o " + out_file;
		if (std::system(cmd.c_str()) != 0) {
			throw std::runtime_error("Linking failed.");
		}
	}
}

int main(const int argc, char **argv) {
	llvm::llvm_shutdown_obj shutdown_obj;

	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	std::print("Initialising Hadron Compiler...\n");

	try {
		auto [input_file, output_file, emit_ir, dump_asm, keep_obj] = parse_args(argc, argv);
		std::cout << "Compiling " << input_file << "...\n";
		std::string source = read_file(input_file);
		hadron::frontend::Lexer lexer(source);
		auto tokens = lexer.tokenize();
		hadron::frontend::Parser parser(std::move(tokens));
		auto unit = parser.parse();

		if (hadron::frontend::Semantic semantic(unit); !semantic.analyze()) {
			std::print(stderr, "Semantic errors:\n");
			for (const auto &err : semantic.errors()) {
				std::cerr << "	" << err << "\n";
			}
			llvm::llvm_shutdown();
			return 1;
		}

		hadron::backend::CodeGenerator code_generator(unit);
		code_generator.generate();
		if (emit_ir)
			code_generator.get_module()->print(llvm::outs(), nullptr);

		std::string obj_file = output_file + ".o";
		code_generator.emit_object(obj_file);

		run_linker(obj_file, output_file);
		std::cout << "Build successful: " << output_file << "\n";

		if (dump_asm) {
			std::print("\n--- Assembly ---\n");
			std::string cmd = "objdump -d -M intel " + output_file;
			std::system(cmd.c_str());
		}

		if (!keep_obj) {
			fs::remove(obj_file);
		}
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
