#pragma once 

#include <vmprofiles.hpp>
#include <memory>
#include <variant>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Value.h>
#include <llvm/Transforms/Utils.h> 
#include <llvm/Transforms/InstCombine/InstCombine.h> 
#include <llvm/Transforms/Scalar.h> 
#include <llvm/Transforms/Scalar/GVN.h> 
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>

//for x86 complie
#include "llvm/Support/Host.h" 
#include "llvm/MC/TargetRegistry.h" 
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Object/ObjectFile.h"

using namespace llvm;

namespace llvm
{
	extern "C" void LLVMInitializeX86TargetInfo();
	extern "C" void LLVMInitializeX86Target();
	extern "C" void LLVMInitializeX86TargetMC();
	extern "C" void LLVMInitializeX86AsmParser();
	extern "C" void LLVMInitializeX86AsmPrinter();
}

namespace lifters {

	class _cvmp2
	{
	public:
		_cvmp2(LLVMContext& context, IRBuilder<>& builder, Module* llvm_module) :context(context), builder(builder), llvm_module(llvm_module) {

			Function* main = Function::Create(FunctionType::get(Type::getVoidTy(context), {}, false), GlobalValue::LinkageTypes::ExternalLinkage, "main", *llvm_module);

			auto bb = BasicBlock::Create(context, "entry", main);
			builder.SetInsertPoint(bb);


			//
			//mov     [rax+rdi], rdx(rax最大已知0xb8)
			//0xb8/8+1 = 24  创建24个8字节虚拟寄存器 (多创建也没问题的,顶多没用到后面被llvm优化)
			//

			for (int i = 0; i < 24; ++i) 
			{
				virtual_registers.push_back(builder.CreateAlloca(llvm::IntegerType::get(context, 64), nullptr, (std::string("vreg") + std::to_string(i)).c_str()));

				builder.CreateStore(ConstantInt::get(Type::getInt64Ty(context), APInt(64, i)), virtual_registers[i]);
			}

			//创建堆栈
			Type* byte_array_type = ArrayType::get(Type::getInt8Ty(context), 0x80);
			virtual_stack = builder.CreateAlloca(byte_array_type, 0, "vsp");



			

		}

		//https://releases.llvm.org/8.0.0/docs/tutorial/LangImpl08.html#full-code-listing
		void complie()
		{
			std::string target_triple = llvm::sys::getDefaultTargetTriple();

			llvm_module->setTargetTriple(target_triple);
			llvm::legacy::PassManager pass;

			llvm::Triple triple(target_triple);
			triple.setObjectFormat(Triple::COFF);

			llvm::LLVMInitializeX86TargetInfo();
			llvm::LLVMInitializeX86Target();
			llvm::LLVMInitializeX86TargetMC();
			llvm::LLVMInitializeX86AsmParser();
			llvm::LLVMInitializeX86AsmPrinter();

			std::string Error;
			llvm::TargetOptions opt;
			auto reloc_model = llvm::Optional< llvm::Reloc::Model>();
			const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, Error);

			llvm::TargetMachine* target_machine = target->createTargetMachine(target_triple, "generic", "", opt, reloc_model);
			llvm_module->setDataLayout(target_machine->createDataLayout());

			llvm::SmallVector< char, 128 > buff;
			llvm::raw_svector_ostream dest(buff);
			target_machine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile);
			auto result = pass.run(*llvm_module);

			std::vector<uint8_t> obj = std::vector<uint8_t>(buff.begin(), buff.end());

			std::ofstream o("main.o", std::ios::binary);
			std::ostream_iterator<uint8_t> oi(o);
			std::copy(obj.begin(), obj.end(), oi);
		}

		LLVMContext& context;
		IRBuilder<>& builder;
		std::unique_ptr<Module> llvm_module;
		std::vector<AllocaInst*> virtual_registers;
		Value* virtual_stack;
		
	};

	struct lifters
	{
		vm::handler::mnemonic_t mnemonic;  //用来定位一个vm handler对应的lifter
		std::function<void(IRBuilder<>&, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>)> hf;
	};

	lifters sregq
	{
		vm::handler::SREGQ,
		//
		//param1(context index)
		//param2(value to store,8byte) 
		//
		[](IRBuilder<> & builder, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param3) {

			uint8_t idx = std::get<uint8_t>(param1);
			uint64_t value_to_be_stored = std::get<uint64_t>(param2);

			






}
	};

	lifters vmexit
	{
		vm::handler::VMEXIT,
		[](IRBuilder<>& builder, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param3) {


		builder.CreateRetVoid();
		builder.ClearInsertionPoint();

	}

	};

	std::map<vm::handler::mnemonic_t, lifters> _h_map
	{
		{vm::handler::SREGQ,sregq},
		{vm::handler::VMEXIT,vmexit},
	};
	

}