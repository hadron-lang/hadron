#include "backend/codegen.hpp"

#include <iostream>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils.h>

namespace hadron::backend {
	CodeGenerator::CodeGenerator(const frontend::CompilationUnit &unit) : unit_(unit) {
		context_ = std::make_unique<llvm::LLVMContext>();
		builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
		std::string moduleName = "hadron_module";
		if (!unit.module.name_path.empty())
			moduleName = std::string(unit_.module.name_path[0].text);
		module_ = std::make_unique<llvm::Module>(moduleName, *context_);
		initialize_passes();
	}

	void CodeGenerator::generate() {
		for (const auto &decl : unit_.declarations)
			gen_stmt(decl);

		llvm::LoopAnalysisManager lam;
		llvm::FunctionAnalysisManager fam;
		llvm::CGSCCAnalysisManager cgam;
		llvm::ModuleAnalysisManager mam;
		llvm::PassBuilder pass_builder;

		pass_builder.registerModuleAnalyses(mam);
		pass_builder.registerCGSCCAnalyses(cgam);
		pass_builder.registerFunctionAnalyses(fam);
		pass_builder.registerLoopAnalyses(lam);
		pass_builder.crossRegisterProxies(lam, fam, cgam, mam);

		llvm::ModulePassManager mpm;
		mpm.addPass(llvm::AlwaysInlinerPass());
		mpm.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::InstCombinePass()));
		mpm.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass()));

		mpm.run(*module_, mam);
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

					llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, s.is_variadic);

					llvm::GlobalValue::LinkageTypes linkage;
					if (s.name.text == "main" || s.is_extern)
						linkage = llvm::Function::ExternalLinkage;
					else
						linkage = llvm::Function::InternalLinkage;

					llvm::Function *function =
						llvm::Function::Create(funcType, linkage, std::string(s.name.text), module_.get());

					if (s.is_extern)
						return;

					function->addFnAttr(llvm::Attribute::AlwaysInline);

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

					if (llvm::verifyFunction(*function, &llvm::errs()))
						std::cerr << "Critical Error: Generated function '" << std::string(s.name.text)
								  << "' is invalid !\n";

					fpm_->run(*function);
				},
				[&](const frontend::VarDeclStmt &s) {
					llvm::Value *initVal;
					if (s.initializer)
						initVal = gen_expr(*s.initializer);
					else
						initVal = llvm::ConstantInt::get(*context_, llvm::APInt(32, 0));

					if (!initVal) {
						std::cerr << "CodeGen Error: Failed to generate initializer for '" << std::string(s.name.text)
								  << "'\n";
						return;
					}

					llvm::Type *varType = initVal->getType();
					if (s.type_annotation) {
						varType = get_llvm_type(*s.type_annotation);
						if (initVal->getType() != varType)
							initVal = builder_->CreateIntCast(initVal, varType, false);
					}
					llvm::Function *func = builder_->GetInsertBlock()->getParent();
					llvm::AllocaInst *alloca = create_entry_block_alloca(func, std::string(s.name.text), varType);
					builder_->CreateStore(initVal, alloca);
					named_values_[std::string(s.name.text)] = alloca;
				},
				[&](const frontend::ReturnStmt &s) {
					if (s.value) {
						llvm::Value *retVal = gen_expr(*s.value);
						if (!retVal)
							return;
						const llvm::Function *func = builder_->GetInsertBlock()->getParent();
						if (llvm::Type *expectedType = func->getReturnType(); retVal->getType() != expectedType)
							retVal = builder_->CreateIntCast(retVal, expectedType, true);
						builder_->CreateRet(retVal);
					} else
						builder_->CreateRetVoid();
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
					// tods: clean management of scopes
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
					if (e.value.type == frontend::TokenType::Number)
						return llvm::ConstantInt::get(*context_, llvm::APInt(64, e.value.text, 10));
					if (e.value.type == frontend::TokenType::String) {
						const std::string_view raw = e.value.text.substr(1, e.value.text.size() - 2);
						const std::string content = resolve_escapes(raw);
						return builder_->CreateGlobalString(content);
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
				[&](const frontend::UnaryExpr &e) -> llvm::Value * {
					llvm::Value *operand = gen_expr(*e.right);
					if (!operand)
						return nullptr;
					switch (e.op.type) {
					case frontend::TokenType::Minus:
						return builder_->CreateNeg(operand, "negtmp");
					case frontend::TokenType::Bang:
						return builder_->CreateNot(operand, "nottmp");
					case frontend::TokenType::Ampersand:
						return gen_addr(*e.right);
					case frontend::TokenType::Star: {
						llvm::Value *ptr = gen_addr(*e.right);
						if (!ptr)
							return nullptr;
						// We need to know the type pointed to in order to load it.
						// LLVM opaque pointers tip: we load the type corresponding to the pointer.
						// Note: In a perfect implementation, we would use the AST type.
						// Here, to simplify things, we assume i64 for the example or rely on the context.
						// Problem: With Opaque Pointers, LoadInst needs the type explicitly.
						// TEMPORARY HACK: We assume i64 for the printf pointer test,
						// but ideally we should pass the AST Type to gen_expr.
						// For now, let's trust the default i32/i64 types.
						return builder_->CreateLoad(builder_->getInt64Ty(), ptr);
					}
					default:
						return nullptr;
					}
				},
				[&](const frontend::GroupingExpr &e) -> llvm::Value * { return gen_expr(*e.expression); },
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

						if (val->getType() != alloca->getAllocatedType())
							val = builder_->CreateIntCast(val, alloca->getAllocatedType(), false);
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
				[&](const frontend::CallExpr &e) -> llvm::Value * {
					if (!std::holds_alternative<frontend::VariableExpr>(e.callee->kind)) {
						std::cerr << "CodeGen Error: Only direct function calls are supported for now.\n";
						return nullptr;
					}

					const auto &[name] = std::get<frontend::VariableExpr>(e.callee->kind);
					const std::string funcName = std::string(name.text);
					llvm::Function *calleeF = module_->getFunction(funcName);
					if (!calleeF) {
						std::cerr << "CodeGen Error: Unknown function referenced '" << funcName << "'.\n";
						return nullptr;
					}

					if (calleeF->isVarArg()) {
						if (e.args.size() < calleeF->arg_size()) {
							std::cerr << "CodeGen Error: Variadic call missing fixed args.\n";
							return nullptr;
						}
					} else {
						if (e.args.size() != calleeF->arg_size()) {
							std::cerr << "CodeGen Error: Argument count mismatch.\n";
							return nullptr;
						}
					}

					std::vector<llvm::Value *> argsV;
					for (unsigned int i = 0; i < e.args.size(); ++i) {
						llvm::Value *argVal = gen_expr(e.args[i]);
						if (!argVal)
							return nullptr;
						if (i < calleeF->arg_size()) {
							if (llvm::Type *expectedType = calleeF->getArg(i)->getType();
								argVal->getType() != expectedType) {
								argVal = builder_->CreateIntCast(argVal, expectedType, true);
							}
						}
						argsV.push_back(argVal);
					}

					if (calleeF->getReturnType()->isVoidTy())
						return builder_->CreateCall(calleeF, argsV);

					return builder_->CreateCall(calleeF, argsV, "calltmp");
				},
				[](const auto &) -> llvm::Value * { return nullptr; }
			},
			expr.kind
		);
	}

	llvm::Value *CodeGenerator::gen_addr(const frontend::Expr &expr) {
		return std::visit(
			overloaded{
				[&](const frontend::VariableExpr &e) -> llvm::Value * {
					const std::string name = std::string(e.name.text);
					if (named_values_.contains(name))
						return named_values_[name];
					std::cerr << "CodeGen Error: Unknown variable '" << name << "'\n";
					return nullptr;
				},
				[&](const frontend::UnaryExpr &e) -> llvm::Value * {
					if (e.op.type == frontend::TokenType::Star)
						return gen_expr(*e.right);
					std::cerr << "CodeGen Error: Cannot take address of this unary expression.\n";
					return nullptr;
				},
				[&](const auto &) -> llvm::Value * {
					std::cerr << "CodeGen Error: Cannot take address of r-value.\n";
					return nullptr;
				}
			},
			expr.kind
		);
	}

	llvm::Type *CodeGenerator::get_llvm_type(const frontend::Type &type) const {
		if (std::holds_alternative<frontend::NamedType>(type.kind)) {
			const auto &[name_path, generic_args] = std::get<frontend::NamedType>(type.kind);
			const std::string_view name = name_path[0].text;

			if (name == "i8" || name == "u8")
				return llvm::Type::getInt8Ty(*context_);
			if (name == "i16" || name == "u16")
				return llvm::Type::getInt16Ty(*context_);
			if (name == "i32" || name == "u32")
				return llvm::Type::getInt32Ty(*context_);
			if (name == "i64" || name == "u64" || name == "usize")
				return llvm::Type::getInt64Ty(*context_);
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

		if (std::holds_alternative<frontend::PointerType>(type.kind))
			return llvm::PointerType::get(*context_, 0);

		return llvm::Type::getVoidTy(*context_);
	}

	void CodeGenerator::initialize_passes() {
		fpm_ = std::make_unique<llvm::legacy::FunctionPassManager>(module_.get());
		fpm_->add(llvm::createPromoteMemoryToRegisterPass());
		fpm_->add(llvm::createInstructionCombiningPass());
		fpm_->add(llvm::createReassociatePass());
		fpm_->add(llvm::createEarlyCSEPass());
		fpm_->add(llvm::createCFGSimplificationPass());
		fpm_->doInitialization();
	}

	llvm::AllocaInst *
	CodeGenerator::create_entry_block_alloca(llvm::Function *func, const llvm::StringRef varName, llvm::Type *type) {
		llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
		return tmpBuilder.CreateAlloca(type, nullptr, varName);
	}

	std::string CodeGenerator::resolve_escapes(const std::string_view src) {
		std::string dest;
		dest.reserve(src.size());
		for (size_t i = 0; i < src.size(); ++i) {
			if (src[i] == '\\' && i + 1 < src.size()) {
				switch (src[i + 1]) {
				case 'n':
					dest += '\n';
					break;
				case 'r':
					dest += '\r';
					break;
				case 't':
					dest += '\t';
					break;
				case '\\':
					dest += '\\';
					break;
				case '"':
					dest += '"';
					break;
				case '0':
					dest += '\0';
					break;
				default:
					dest += src[i];
					continue;
				}
				i++;
			} else
				dest += src[i];
		}
		return dest;
	}
} // namespace hadron::backend
