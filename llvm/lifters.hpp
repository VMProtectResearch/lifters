#pragma once 

#include <vmprofiles.hpp>

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


namespace lifters {

	struct lifters
	{
		vm::handler::mnemonic_t mnemonic;  //用来定位一个vm handler对应的lifter

		std::function<void(llvm::BasicBlock,llvm::IRBuilder<>,uint64_t,uint64_t,uint64_t)> _f;   //处理basic block


	};


	

}