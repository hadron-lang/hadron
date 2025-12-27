#include "backend/codegen.hpp"

#include <iostream>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

namespace hadron::backend {
	CodeGenerator::CodeGenerator(const frontend::CompilationUnit &unit) : unit_(unit) {
		context_ = std::make_unique<llvm::LLVMContext>();
		builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
		module_ = std::make_unique<llvm::Module>(unit_.module.name_path[0].text, *context_);
	}

	void CodeGenerator::generate() {
		for (const auto &decl : unit_.declarations)
			gen_stmt(decl);

		module_->print(llvm::outs(), nullptr);
	}

	void CodeGenerator::emit_object(const std::string &filename) const {
		const auto targetTripleStr = llvm::sys::getDefaultTargetTriple();
		const llvm::Triple targetTriple(targetTripleStr);
		module_->setTargetTriple(targetTriple);

		std::string error;
		const auto target = llvm::TargetRegistry::lookupTarget(targetTripleStr, error);
		if (!target) {
			std::cerr << "Error: " << error << "\n";
			return;
		}

		const auto cpu = "generic";
		const auto features = "";

		const llvm::TargetOptions options;
		constexpr auto rm = std::optional<llvm::Reloc::Model>();
		const std::unique_ptr<llvm::TargetMachine> targetMachine(
			target->createTargetMachine(targetTriple, cpu, features, options, rm)
		);

		if (!targetMachine) {
			std::cerr << "Error: Could not create TargetMachine\n";
			return;
		}

		module_->setDataLayout(targetMachine->createDataLayout());

		std::error_code ec;
		llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
		if (ec) {
			std::cerr << "Could not open file: " << ec.message() << "\n";
			return;
		}

		llvm::legacy::PassManager pass;
		if (constexpr auto fileType = llvm::CodeGenFileType::ObjectFile;
			targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
			std::cerr << "TargetMachine can't emit a file of this type\n";
			return;
		}

		pass.run(*module_);
		dest.flush();

		std::cout << "Wrote object file to " << filename << "\n";
	}

	template <class... Ts> struct overloaded : Ts... {
		using Ts::operator()...;
	};
	template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	void CodeGenerator::gen_stmt(const frontend::Stmt &stmt) {
		std::visit(
			overloaded{
				[&](const frontend::FunctionDecl &s) {
					named_values_.clear();
					loops_.clear();
					std::vector<llvm::Type *> paramTypes;
					for (const auto &[name, type] : s.params)
						paramTypes.push_back(get_llvm_type(type));

					llvm::Type *returnType =
						s.return_type ? get_llvm_type(*s.return_type) : llvm::Type::getVoidTy(*context_);

					llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);
					llvm::Function *function = llvm::Function::Create(
						funcType, llvm::Function::ExternalLinkage, std::string(s.name.text), module_.get()
					);

					llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(*context_, "entry", function);
					builder_->SetInsertPoint(entryBlock);

					unsigned idx = 0;
					for (auto &arg : function->args()) {
						std::string argName = std::string(s.params[idx].name.text);
						arg.setName(argName);
						llvm::AllocaInst *alloca = create_entry_block_alloca(function, argName, arg.getType());
						builder_->CreateStore(&arg, alloca);
						named_values_[argName] = alloca;
						idx++;
					}

					for (const auto &bodyStmt : s.body)
						gen_stmt(bodyStmt);

					if (returnType->isVoidTy() && !entryBlock->getTerminator())
						builder_->CreateRetVoid();
				},
				[&](const frontend::VarDeclStmt &s) {
					llvm::Value *initVal;
					if (s.initializer)
						initVal = gen_expr(*s.initializer);
					else {
						// todo: default value by type
						initVal = llvm::ConstantInt::get(*context_, llvm::APInt(32, 0));
					}
					llvm::Function *func = builder_->GetInsertBlock()->getParent();
					llvm::AllocaInst *alloca =
						create_entry_block_alloca(func, std::string(s.name.text), initVal->getType());
					builder_->CreateStore(initVal, alloca);
					named_values_[std::string(s.name.text)] = alloca;
				},
				[&](const frontend::ReturnStmt &s) {
					if (s.value) {
						llvm::Value *retVal = gen_expr(*s.value);
						builder_->CreateRet(retVal);
					} else {
						builder_->CreateRetVoid();
					}
				},
				[&](const frontend::IfStmt &s) {
					llvm::Value *condV = gen_expr(s.condition);
					if (!condV)
						return;
					llvm::Function *func = builder_->GetInsertBlock()->getParent();
					llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*context_, "then", func);
					llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*context_, "else");
					llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context_, "ifcont");
					llvm::BasicBlock *actualElseBB = s.else_branch ? elseBB : mergeBB;
					builder_->CreateCondBr(condV, thenBB, actualElseBB);
					builder_->SetInsertPoint(thenBB);
					if (s.then_branch)
						gen_stmt(*s.then_branch);
					if (!builder_->GetInsertBlock()->getTerminator())
						builder_->CreateBr(mergeBB);
					if (s.else_branch) {
						func->insert(func->end(), elseBB);
						builder_->SetInsertPoint(elseBB);
						gen_stmt(*s.else_branch);
						if (!builder_->GetInsertBlock()->getTerminator())
							builder_->CreateBr(mergeBB);
					}
					func->insert(func->end(), mergeBB);
					builder_->SetInsertPoint(mergeBB);
				},
				[&](const frontend::WhileStmt &s) {
					llvm::Function *func = builder_->GetInsertBlock()->getParent();
					llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context_, "cond", func);
					llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(*context_, "loop");
					llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context_, "afterloop");
					builder_->CreateBr(condBB);
					builder_->SetInsertPoint(condBB);
					llvm::Value *condV = gen_expr(s.condition);
					builder_->CreateCondBr(condV, loopBB, afterBB);
					func->insert(func->end(), loopBB);
					builder_->SetInsertPoint(loopBB);
					loops_.push_back({condBB, afterBB});
					if (s.body)
						gen_stmt(*s.body);
					loops_.pop_back();
					builder_->CreateBr(condBB);
					func->insert(func->end(), afterBB);
					builder_->SetInsertPoint(afterBB);
				},
				[&](const frontend::LoopStmt &s) {
					llvm::Function *func = builder_->GetInsertBlock()->getParent();
					llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(*context_, "loop", func);
					llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context_, "afterloop");
					builder_->CreateBr(loopBB);
					builder_->SetInsertPoint(loopBB);
					loops_.push_back({loopBB, afterBB});
					if (s.body)
						gen_stmt(*s.body);
					loops_.pop_back();
					builder_->CreateBr(loopBB);
					func->insert(func->end(), afterBB);
					builder_->SetInsertPoint(afterBB);
				},
				[&](const frontend::BreakStmt &) {
					if (loops_.empty())
						return;
					builder_->CreateBr(loops_.back().breakBlock);
					llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(*context_, "dead");
					builder_->GetInsertBlock()->getParent()->insert(
						builder_->GetInsertBlock()->getParent()->end(), deadBB
					);
					builder_->SetInsertPoint(deadBB);
				},
				[&](const frontend::ContinueStmt &) {
					if (loops_.empty())
						return;
					builder_->CreateBr(loops_.back().continueBlock);
					llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(*context_, "dead");
					builder_->GetInsertBlock()->getParent()->insert(
						builder_->GetInsertBlock()->getParent()->end(), deadBB
					);
					builder_->SetInsertPoint(deadBB);
				},
				[&](const frontend::ForStmt &s) {
					// TODO: clean management of scopes
					if (s.initializer)
						gen_stmt(*s.initializer);
					llvm::Function *func = builder_->GetInsertBlock()->getParent();
					llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context_, "cond", func);
					llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*context_, "body");
					llvm::BasicBlock *incrBB = llvm::BasicBlock::Create(*context_, "incr");
					llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context_, "after");
					builder_->CreateBr(condBB);
					builder_->SetInsertPoint(condBB);
					if (s.condition) {
						llvm::Value *condV = gen_expr(*s.condition);
						builder_->CreateCondBr(condV, bodyBB, afterBB);
					} else {
						builder_->CreateBr(bodyBB);
					}

					func->insert(func->end(), bodyBB);
					builder_->SetInsertPoint(bodyBB);
					loops_.push_back({incrBB, afterBB});
					if (s.body)
						gen_stmt(*s.body);
					loops_.pop_back();
					builder_->CreateBr(incrBB);
					func->insert(func->end(), incrBB);
					builder_->SetInsertPoint(incrBB);
					if (s.increment)
						gen_expr(*s.increment);
					builder_->CreateBr(condBB);
					func->insert(func->end(), afterBB);
					builder_->SetInsertPoint(afterBB);
				},
				[&](const frontend::BlockStmt &s) {
					// todo: manage scope (push/pop named_values_)
					for (const auto &subStmt : s.statements)
						gen_stmt(subStmt);
				},
				[&](const frontend::ExpressionStmt &s) { gen_expr(s.expression); },
				[](const auto &) {}
			},
			stmt.kind
		);
	}

	llvm::Value *CodeGenerator::gen_expr(const frontend::Expr &expr) {
		return std::visit(
			overloaded{
				[&](const frontend::LiteralExpr &e) -> llvm::Value * {
					if (e.value.type == frontend::TokenType::Number) {
						const int val = std::stoi(std::string(e.value.text));
						return llvm::ConstantInt::get(*context_, llvm::APInt(32, static_cast<uint32_t>(val), true));
					}
					if (e.value.type == frontend::TokenType::KwTrue)
						return llvm::ConstantInt::get(*context_, llvm::APInt(1, 1));
					if (e.value.type == frontend::TokenType::KwFalse)
						return llvm::ConstantInt::get(*context_, llvm::APInt(1, 0));
					return nullptr;
				},
				[&](const frontend::VariableExpr &e) -> llvm::Value * {
					const std::string name = std::string(e.name.text);
					llvm::AllocaInst *alloca = named_values_[name];
					if (!alloca) {
						std::cerr << "CodeGen Error: Unknown variable " << name << "\n";
						return nullptr;
					}
					return builder_->CreateLoad(alloca->getAllocatedType(), alloca, name.c_str());
				},
				[&](const frontend::BinaryExpr &e) -> llvm::Value * {
					if (e.op.type == frontend::TokenType::Eq) {
						if (!std::holds_alternative<frontend::VariableExpr>(e.left->kind)) {
							std::cerr << "CodeGen Error: LHS of assignment must be a variable.\n";
							return nullptr;
						}

						const auto &[name] = std::get<frontend::VariableExpr>(e.left->kind);
						const std::string t_name = std::string(name.text);
						llvm::Value *val = gen_expr(*e.right);
						if (!val)
							return nullptr;

						llvm::AllocaInst *alloca = named_values_[t_name];
						if (!alloca) {
							std::cerr << "CodeGen Error: Unknown variable " << t_name << "\n";
							return nullptr;
						}

						builder_->CreateStore(val, alloca);
						return val;
					}

					llvm::Value *L = gen_expr(*e.left);
					llvm::Value *R = gen_expr(*e.right);
					if (!L || !R)
						return nullptr;

					switch (e.op.type) {
					case frontend::TokenType::Plus:
						return builder_->CreateAdd(L, R, "addtmp");
					case frontend::TokenType::Minus:
						return builder_->CreateSub(L, R, "subtmp");
					case frontend::TokenType::Star:
						return builder_->CreateMul(L, R, "multmp");
					case frontend::TokenType::Slash:
						return builder_->CreateSDiv(L, R, "divtmp");
					case frontend::TokenType::EqEq:
						return builder_->CreateICmpEQ(L, R, "eqtmp");
					case frontend::TokenType::Lt:
						return builder_->CreateICmpSLT(L, R, "lttmp");
					case frontend::TokenType::Gt:
						return builder_->CreateICmpSGT(L, R, "gttmp");
					// todo: add LtEq, GtEq, BangEq...
					default:
						return nullptr;
					}
				},
				[&](const frontend::CallExpr &e) -> llvm::Value * { return nullptr; },
				[](const auto &) -> llvm::Value * { return nullptr; }
			},
			expr.kind
		);
	}

	llvm::Type *CodeGenerator::get_llvm_type(const frontend::Type &type) const {
		if (std::holds_alternative<frontend::NamedType>(type.kind)) {
			const auto &[name_path, generic_args] = std::get<frontend::NamedType>(type.kind);
			const std::string_view name = name_path[0].text;

			if (name == "i8")
				return llvm::Type::getInt8Ty(*context_);
			if (name == "i16")
				return llvm::Type::getInt16Ty(*context_);
			if (name == "i32")
				return llvm::Type::getInt32Ty(*context_);
			if (name == "i64")
				return llvm::Type::getInt64Ty(*context_);
			/*if (name == "u8")
				return llvm::Type::getInt8Ty(*context_);
			if (name == "u16")
				return llvm::Type::getInt16Ty(*context_);
			if (name == "u32")
				return llvm::Type::getInt32Ty(*context_);
			if (name == "u64")
				return llvm::Type::getInt64Ty(*context_);*/
			if (name == "bool")
				return llvm::Type::getInt1Ty(*context_);
			if (name == "byte")
				return llvm::Type::getInt1Ty(*context_);
			if (name == "void")
				return llvm::Type::getVoidTy(*context_);
			if (name == "f32")
				return llvm::Type::getFloatTy(*context_);
			if (name == "f64")
				return llvm::Type::getDoubleTy(*context_);
		}

		// todo: default void for avoid the crash (to change)
		return llvm::Type::getVoidTy(*context_);
	}

	llvm::AllocaInst *
	CodeGenerator::create_entry_block_alloca(llvm::Function *func, const llvm::StringRef varName, llvm::Type *type) {
		llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
		return tmpBuilder.CreateAlloca(type, nullptr, varName);
	}
} // namespace hadron::backend
