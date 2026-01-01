#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>

#include "frontend/ast.hpp"

namespace hadron::backend {

	struct LoopContext {
		llvm::BasicBlock *continueBlock;
		llvm::BasicBlock *breakBlock;
	};

	class CodeGenerator {
		const frontend::CompilationUnit &unit_;
		std::unique_ptr<llvm::LLVMContext> context_;
		std::unique_ptr<llvm::Module> module_;
		std::unique_ptr<llvm::IRBuilder<>> builder_;
		std::unique_ptr<llvm::legacy::FunctionPassManager> fpm_;
		std::map<std::string, llvm::AllocaInst *> named_values_;
		std::vector<LoopContext> loops_;
		std::unordered_map<std::string, std::unordered_map<std::string, u32>> struct_field_indices_;

		void gen_stmt(const frontend::Stmt &stmt);

		llvm::Value *gen_expr(const frontend::Expr &expr);

		llvm::Value *gen_addr(const frontend::Expr &expr);

		llvm::Type *get_llvm_type(const frontend::Type &type) const;

		void initialize_passes();

		static llvm::AllocaInst *
		create_entry_block_alloca(llvm::Function *func, llvm::StringRef varName, llvm::Type *type);

		static std::string resolve_escapes(std::string_view src);

	public:
		explicit CodeGenerator(const frontend::CompilationUnit &unit);

		void generate();

		void emit_object(const std::string &filename) const;

		llvm::Module *get_module() const {
			return module_.get();
		}
	};
} // namespace hadron::backend
