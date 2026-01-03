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

#include "backend/codegen.hpp"
#include "frontend/utils.hpp"

#ifndef NDEBUG
#define TRACE(x) std::cout << "[CodeGen] " << x << "\n"
#else // #ifndef NDEBUG
#define TRACE(x)
#endif // #ifndef NDEBUG else

namespace hadron::backend {
	CodeGenerator::CodeGenerator(const frontend::CompilationUnit &unit) : unit_(unit) {
		context_ = std::make_unique<llvm::LLVMContext>();
		builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);

		std::string moduleName = "hadron_module";
		if (!unit.module.name_path.empty())
			moduleName = std::string(unit_.module.name_path[0].text);
		module_ = std::make_unique<llvm::Module>(moduleName, *context_);

		std::string error;
		const auto targetTripleStr = llvm::sys::getDefaultTargetTriple();
		const llvm::Triple targetTriple(targetTripleStr);

		if (const auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error)) {
			const auto cpu = "generic";
			const auto features = "";
			const llvm::TargetOptions options;
			constexpr auto rm = std::optional<llvm::Reloc::Model>();
			const auto targetMachine = std::unique_ptr<llvm::TargetMachine>(
				target->createTargetMachine(targetTriple, cpu, features, options, rm)
			);

			module_->setDataLayout(targetMachine->createDataLayout());
			module_->setTargetTriple(targetTriple);
		} else {
			std::cerr << "CodeGen Warning: Failed to lookup target: " << error << "\n";
		}

		initialize_passes();
	}

	void CodeGenerator::generate() {
		TRACE("Starting generation...");
		for (const auto &decl : unit_.declarations)
			gen_stmt(decl);

		TRACE("Verification step...");
		if (llvm::verifyModule(*module_, &llvm::errs())) {
			std::cerr << "Module verification failed!\n";
		}

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
		TRACE("Generation finished.");
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
		if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
			std::cerr << "TargetMachine can't emit a file of this type\n";
			return;
		}

		pass.run(*module_);
		dest.flush();
		TRACE("Object file emitted to " << filename);
	}

	template <class... Ts> struct overloaded : Ts... {
		using Ts::operator()...;
	};
	template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	void CodeGenerator::gen_stmt(const frontend::Stmt &stmt) {
		std::visit(
			overloaded{
				[&](const frontend::FunctionDecl &s) {
					TRACE("Gen Function: " << s.name.text);
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

					llvm::Function *function = module_->getFunction(std::string(s.name.text));
					if (!function)
						function = llvm::Function::Create(funcType, linkage, std::string(s.name.text), module_.get());

					if (s.is_extern)
						return;

					function->addFnAttr(llvm::Attribute::AlwaysInline);

					llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(*context_, "entry", function);
					builder_->SetInsertPoint(entryBlock);

					unsigned idx = 0;
					for (auto &arg : function->args()) {
						if (idx < s.params.size()) {
							std::string argName = std::string(s.params[idx].name.text);
							arg.setName(argName);
							llvm::AllocaInst *alloca = create_entry_block_alloca(function, argName, arg.getType());
							builder_->CreateStore(&arg, alloca);
							named_values_[argName] = alloca;
						}
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
				[&](const frontend::StructDecl &s) {
					TRACE("Gen Struct: " << s.name.text);
					const std::string structName = std::string(s.name.text);
					llvm::StructType *structType = llvm::StructType::getTypeByName(*context_, "struct." + structName);
					if (!structType)
						structType = llvm::StructType::create(*context_, "struct." + structName);

					u32 index = 0;
					std::vector<llvm::Type *> fieldTypes;
					for (const auto &[name, type] : s.fields) {
						struct_field_indices_[structName][std::string(name.text)] = index++;
						fieldTypes.push_back(get_llvm_type(type));
					}

					if (structType->isOpaque())
						structType->setBody(fieldTypes, false);
				},
				[&](const frontend::VarDeclStmt &s) {
					TRACE("Gen Var: " << s.name.text);
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
						if (llvm::Type *expectedType = func->getReturnType(); retVal->getType() != expectedType) {
							if (retVal->getType()->isIntegerTy() && expectedType->isIntegerTy())
								retVal = builder_->CreateIntCast(retVal, expectedType, true);
						}
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
					switch (e.op.type) {
					case frontend::TokenType::Minus: {
						llvm::Value *operand = gen_expr(*e.right);
						return operand ? builder_->CreateNeg(operand, "negtmp") : nullptr;
					}
					case frontend::TokenType::Bang: {
						llvm::Value *operand = gen_expr(*e.right);
						return operand ? builder_->CreateNot(operand, "nottmp") : nullptr;
					}
					case frontend::TokenType::Ampersand: {
						if (std::holds_alternative<frontend::GetExpr>(e.right->kind))
							return gen_addr(*e.right);
						if (std::holds_alternative<frontend::VariableExpr>(e.right->kind)) {
							const auto &[name] = std::get<frontend::VariableExpr>(e.right->kind);
							return named_values_[std::string(name.text)];
						}
						if (std::holds_alternative<frontend::ArrayAccessExpr>(e.right->kind))
							return gen_addr(*e.right);
						std::cerr << "CodeGen Error: Can only take address of variables or fields.\n";
						return nullptr;
					}
					case frontend::TokenType::Star: {
						llvm::Value *ptr = gen_expr(*e.right);
						if (!ptr)
							return nullptr;
						// todo: Use the actual pointed type via the AST when available
						return builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), ptr, "deref");
					}
					default:
						return nullptr;
					}
				},
				[&](const frontend::GetExpr &e) -> llvm::Value * {
					llvm::Value *ptr = gen_addr(expr);
					if (!ptr || !expr.type_cache)
						return nullptr;
					return builder_->CreateLoad(get_llvm_type(*expr.type_cache), ptr, "fld");
				},
				[&](const frontend::CastExpr &e) -> llvm::Value * {
					llvm::Value *val = gen_expr(*e.expr);
					if (!val)
						return nullptr;

					llvm::Type *destTy = get_llvm_type(e.target_type);
					const llvm::Type *srcTy = val->getType();

					if (srcTy == destTy)
						return val;

					if (srcTy->isPointerTy() && destTy->isPointerTy())
						return builder_->CreateBitCast(val, destTy);

					if (srcTy->isPointerTy() && destTy->isIntegerTy())
						return builder_->CreatePtrToInt(val, destTy);

					if (srcTy->isIntegerTy() && destTy->isPointerTy())
						return builder_->CreateIntToPtr(val, destTy);

					if (srcTy->isIntegerTy() && destTy->isIntegerTy()) {
						if (srcTy->getIntegerBitWidth() > destTy->getIntegerBitWidth())
							return builder_->CreateTrunc(val, destTy);
						return builder_->CreateZExt(val, destTy);
					}

					return val;
				},
				[&](const frontend::GroupingExpr &e) -> llvm::Value * { return gen_expr(*e.expression); },
				[&](const frontend::BinaryExpr &e) -> llvm::Value * {
					if (e.op.type == frontend::TokenType::Eq) {
						llvm::Value *ptr = nullptr;
						llvm::Type *destType = nullptr;

						if (std::holds_alternative<frontend::VariableExpr>(e.left->kind)) {
							const auto &[name] = std::get<frontend::VariableExpr>(e.left->kind);
							if (const auto alloca = named_values_[std::string(name.text)]) {
								ptr = alloca;
								destType = alloca->getAllocatedType();
							}
						} else if (std::holds_alternative<frontend::GetExpr>(e.left->kind)) {
							ptr = gen_addr(*e.left);
							if (ptr && e.left->type_cache)
								destType = get_llvm_type(*e.left->type_cache);
						} else if (std::holds_alternative<frontend::ArrayAccessExpr>(e.left->kind)) {
							ptr = gen_addr(*e.left);
							if (ptr && e.left->type_cache)
								destType = get_llvm_type(*e.left->type_cache);
						}

						if (!ptr || !destType) {
							std::cerr << "CodeGen Error: LHS of assignment must be a variable or field.\n";
							return nullptr;
						}

						llvm::Value *val = gen_expr(*e.right);
						if (!val)
							return nullptr;

						if (val->getType() != destType) {
							if (destType->isPointerTy() && val->getType()->isIntegerTy())
								val = builder_->CreateIntToPtr(val, destType);
							else if (destType->isPointerTy() && val->getType()->isPointerTy())
								val = builder_->CreateBitCast(val, destType);
							else if (destType->isIntegerTy() && val->getType()->isIntegerTy())
								val = builder_->CreateIntCast(val, destType, false);
						}
						builder_->CreateStore(val, ptr);
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
					default:
						return nullptr;
					}
				},
				[&](const frontend::CallExpr &e) -> llvm::Value * {
					if (!std::holds_alternative<frontend::VariableExpr>(e.callee->kind)) {
						std::cerr << "CodeGen Error: Only direct calls supported.\n";
						return nullptr;
					}

					const auto &[name] = std::get<frontend::VariableExpr>(e.callee->kind);
					llvm::Function *calleeF = module_->getFunction(name.text);
					if (!calleeF) {
						std::cerr << "CodeGen Error: Unknown function.\n";
						return nullptr;
					}

					std::vector<llvm::Value *> argsV;
					for (unsigned int i = 0; i < e.args.size(); ++i) {
						llvm::Value *argVal = gen_expr(e.args[i]);
						if (!argVal)
							return nullptr;

						if (i < calleeF->arg_size()) {
							if (llvm::Type *expectedType = calleeF->getArg(i)->getType();
								argVal->getType() != expectedType) {
								if (expectedType->isPointerTy() && argVal->getType()->isPointerTy())
									argVal = builder_->CreateBitCast(argVal, expectedType);
								else {
									argVal = builder_->CreateIntCast(argVal, expectedType, true);
								}
							}
						}
						argsV.push_back(argVal);
					}

					if (calleeF->getReturnType()->isVoidTy())
						return builder_->CreateCall(calleeF, argsV);

					return builder_->CreateCall(calleeF, argsV, "calltmp");
				},
				[&](const frontend::SizeOfExpr &e) -> llvm::Value * {
					llvm::Type *llvmType = get_llvm_type(e.type);
					const u64 size = module_->getDataLayout().getTypeAllocSize(llvmType);
					return llvm::ConstantInt::get(*context_, llvm::APInt(64, size));
				},
				[&](const frontend::StructInitExpr &e) -> llvm::Value * {
					llvm::Type *structTy = get_llvm_type(e.type);
					if (!structTy)
						return nullptr;

					llvm::AllocaInst *alloc = builder_->CreateAlloca(structTy, nullptr, "struct_init");
					const std::string structName = frontend::get_type_name(e.type);
					if (structName.empty() || !struct_field_indices_.contains(structName)) {
						std::cerr << "CodeGen Error: Unknown struct layout for " << structName << "\n";
						return nullptr;
					}

					for (const auto &[name, value] : e.fields) {
						llvm::Value *val = gen_expr(*value);
						if (!val)
							continue;

						std::string fieldName = std::string(name.text);
						const u32 index = struct_field_indices_[structName][fieldName];

						llvm::Value *fieldPtr = builder_->CreateStructGEP(structTy, alloc, index, "init_" + fieldName);
						if (llvm::Type *destType = fieldPtr->getType(); val->getType() != destType) {
							if (destType->isPointerTy() && val->getType()->isPointerTy())
								val = builder_->CreateBitCast(val, destType);
							else if (destType->isIntegerTy() && val->getType()->isIntegerTy())
								val = builder_->CreateIntCast(val, destType, false);
						}

						builder_->CreateStore(val, fieldPtr);
					}

					return builder_->CreateLoad(structTy, alloc, "struct_val");
				},
				[&](const frontend::ArrayAccessExpr &e) -> llvm::Value * {
					llvm::Value *ptr = gen_addr(expr);
					if (!ptr || !expr.type_cache)
						return nullptr;
					return builder_->CreateLoad(get_llvm_type(*expr.type_cache), ptr, "array_val");
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
					std::cerr << "CodeGen Error: Cannot take address.\n";
					return nullptr;
				},
				[&](const frontend::GetExpr &e) -> llvm::Value * {
					if (!e.object->type_cache)
						return nullptr;
					frontend::Type objType = *e.object->type_cache;
					bool isPointer = false;
					if (std::holds_alternative<frontend::PointerType>(objType.kind)) {
						isPointer = true;
						objType = *std::get<frontend::PointerType>(objType.kind).inner;
					}

					const std::string structName = frontend::get_type_name(objType);
					const std::string fieldName = std::string(e.name.text);

					if (!struct_field_indices_.contains(structName))
						return nullptr;
					const u32 fieldIndex = struct_field_indices_[structName][fieldName];

					llvm::Value *basePtr = isPointer ? gen_expr(*e.object) : gen_addr(*e.object);
					if (!basePtr)
						return nullptr;

					llvm::StructType *st = llvm::StructType::getTypeByName(*context_, "struct." + structName);
					if (!st)
						return nullptr;

					return builder_->CreateStructGEP(st, basePtr, fieldIndex, "ptr_" + fieldName);
				},
				[&](const frontend::ArrayAccessExpr &e) -> llvm::Value * {
					llvm::Value *basePtr = gen_expr(*e.target);
					if (!basePtr)
						return nullptr;

					llvm::Value *indexVal = gen_expr(*e.index);
					if (!indexVal)
						return nullptr;

					if (!e.target->type_cache)
						return nullptr;

					auto [kind] = *e.target->type_cache;
					if (!std::holds_alternative<frontend::PointerType>(kind))
						return nullptr;

					const frontend::Type elementType = *std::get<frontend::PointerType>(kind).inner;
					llvm::Type *llvmElementType = get_llvm_type(elementType);

					return builder_->CreateGEP(llvmElementType, basePtr, indexVal, "arrayidx");
				},
				[](const auto &) -> llvm::Value * {
					std::cerr << "CodeGen Error: Cannot take address of r-value.\n";
					return nullptr;
				}
			},
			expr.kind
		);
	}

	llvm::Type *CodeGenerator::get_llvm_type(const frontend::Type &type) const {
		if (std::holds_alternative<frontend::BuiltinType>(type.kind)) {
			switch (std::get<frontend::BuiltinType>(type.kind)) {
			case frontend::BuiltinType::I8:
			case frontend::BuiltinType::U8:
			case frontend::BuiltinType::Byte:
				return llvm::Type::getInt8Ty(*context_);
			case frontend::BuiltinType::I16:
			case frontend::BuiltinType::U16:
				return llvm::Type::getInt16Ty(*context_);
			case frontend::BuiltinType::I32:
			case frontend::BuiltinType::U32:
				return llvm::Type::getInt32Ty(*context_);
			case frontend::BuiltinType::I64:
			case frontend::BuiltinType::U64:
				return llvm::Type::getInt64Ty(*context_);
			case frontend::BuiltinType::Bool:
				return llvm::Type::getInt1Ty(*context_);
			case frontend::BuiltinType::F32:
				return llvm::Type::getFloatTy(*context_);
			case frontend::BuiltinType::F64:
				return llvm::Type::getDoubleTy(*context_);
			default:
				return llvm::Type::getVoidTy(*context_);
			}
		}

		if (std::holds_alternative<frontend::NamedType>(type.kind)) {
			const auto &[name_path, generic_args] = std::get<frontend::NamedType>(type.kind);
			const std::string_view name = name_path[0].text;

			if (name == "i8" || name == "u8" || name == "byte")
				return llvm::Type::getInt8Ty(*context_);
			if (name == "i16" || name == "u16")
				return llvm::Type::getInt16Ty(*context_);
			if (name == "i32" || name == "u32")
				return llvm::Type::getInt32Ty(*context_);
			if (name == "i64" || name == "u64" || name == "usize")
				return llvm::Type::getInt64Ty(*context_);
			if (name == "bool")
				return llvm::Type::getInt1Ty(*context_);
			if (name == "void")
				return llvm::Type::getVoidTy(*context_);
			if (name == "f32")
				return llvm::Type::getFloatTy(*context_);
			if (name == "f64")
				return llvm::Type::getDoubleTy(*context_);
			if (const auto st = llvm::StructType::getTypeByName(*context_, "struct." + std::string(name)))
				return st;
		}

		if (std::holds_alternative<frontend::StructType>(type.kind)) {
			const auto &[name, fields] = std::get<frontend::StructType>(type.kind);
			if (const auto lst = llvm::StructType::getTypeByName(*context_, "struct." + name))
				return lst;
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
