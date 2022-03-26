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

using namespace llvm;

using handler_function_type = void(BasicBlock, IRBuilder<>, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>);

namespace lifters {
	class _cvmp2
	{
	public:
		_cvmp2(LLVMContext& context, IRBuilder<>& builder, Module* llvm_module) :context(context), builder(builder), llvm_module(llvm_module) {

		}

		LLVMContext& context;
		IRBuilder<>& builder;
		std::unique_ptr<Module> llvm_module;

	};

	struct lifters
	{
		vm::handler::mnemonic_t mnemonic;  //用来定位一个vm handler对应的lifter

		//void func(BasicBlock, IRBuilder<>,uint64_t param1,uint64_t param2,uint64_t param3)
		std::function<handler_function_type> _f;

	};

	std::map<vm::handler::mnemonic_t, lifters> _h_map
	{
		{}
	}
	

}