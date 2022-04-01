#pragma once 

#include <vmprofiles.hpp>
#include <vmctx.hpp>
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
#include <llvm/IR/InlineAsm.h>
#include <llvm/Support/Alignment.h>

//for x86 complie
#include <llvm/Support/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Object/ObjectFile.h>

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
		_cvmp2(LLVMContext& context, IRBuilder<>& builder, Module* llvm_module,vm::ctx_t& vmctx) :context(context), builder(builder), llvm_module(llvm_module),vmctx(vmctx) {

			//void(*)(int64)
			main = Function::Create(FunctionType::get(Type::getVoidTy(context), {PointerType::getInt64Ty(context)}, false), GlobalValue::LinkageTypes::ExternalLinkage, "main", *llvm_module);

			auto bb = BasicBlock::Create(context, "entry", main);
			builder.SetInsertPoint(bb);

			//
		    //vm-entry的时候会push很多常用寄存器,同样也会不断的调用stregq到context中
		    //这一段应该也是要搞的,不然llvm不认识context中的某一项到底是哪个寄存器,也没有办法执行优化
			//常用寄存器加flag一共18个
			// rax rbx rcx rdx 
			// rsi rdi rip rsp rbp 
			// r8-r15
			// rflags
			// 除了rip都是要保存的
			//
			std::string asm_str;
			char buffer[256]{};
			for (auto& i : vmctx.vm_entry)
			{
				if (i.instr.mnemonic == ZYDIS_MNEMONIC_PUSH || i.instr.mnemonic == ZYDIS_MNEMONIC_PUSHFQ)
				{
					vm::util::print(i.instr, buffer);

					//如果是push一个内存地址的值
					if (i.instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY) {
						asm_str.append(buffer).append(";");
						asm_str.append("mov rax,0;");
						continue;
					}
					asm_str.append(buffer).append(";");
				}
			}
			//asm_str.append("mov rbp,rsp;");

			llvm::InlineAsm* inlineAsm = llvm::InlineAsm::get(llvm::FunctionType::get(Type::getVoidTy(context), false), asm_str, "", false, false, llvm::InlineAsm::AD_Intel);

			//insert asm 
			builder.CreateCall(inlineAsm);


			//
			//mov     [rax+rdi], rdx(rax最大已知0xb8)
			//0xb8/8+1 = 24  创建24个8字节虚拟寄存器
			//
			//sub rsp,0x140(0xc0+0x80)
			/*
			 _____________   PE文件中真实的堆栈图
			|
			|
			|
			|VM_CONTEXT			-> 0xC0  
			|
			|
			|_____________
			|
			|VM_STACK			-> 0x80
			|
			|_____________      -> RBP (virtual stack start)
			|
			|
			|
			|COMMON REGISRER    ->
			|
			|
			|_____________
		
			________________       llvm编译后生成的堆栈图 
			|
			|
			|
			|
			|
			|
			|COMMON REGISRER
			|
			|
			|
			|
			|_______________
			|
			|
			|
			|VM_CONTEXT
			|
			|
			|_______________
			|
			|
			|
			|VM_STACK(暂时不用，挪到最上面去了)
			|
			|
			|_______________
			
			


			*/


			for (int i = 0; i < 24; ++i) 
			{
				virtual_registers.push_back(builder.CreateAlloca(llvm::IntegerType::get(context, 64), nullptr, (std::string("vreg") + std::to_string(i)).c_str()));

				//builder.CreateStore(ConstantInt::get(Type::getInt64Ty(context), APInt(64, i)), virtual_registers[i]);
			}

			insert_asm_locally("mov rcx,rsp;");
			// i64
			stack = main->getArg(0);
			outs() << "[-]stack type : ";
			stack->getType()->print(outs());
			outs() << "\n";


			//创建堆栈
			
			/* llvm complier (stack)
			
			 _______________          
			|
			|
			|
			|
			|
			|
			|COMMON REGISRER
			|
			|
			|
			|
			|_______________
			|
			|
			|
			|VM_CONTEXT
			|
			|
			|_______________
			|
			|
			|
			|
			|
			|
			|_______________
			
			
			
			*/
			







		}

		void add_optimize()
		{
			std::unique_ptr<llvm::legacy::FunctionPassManager> fpm = std::make_unique<llvm::legacy::FunctionPassManager>(&(*llvm_module));

			//some optimizes

			fpm->add(llvm::createDeadStoreEliminationPass());
			fpm->add(llvm::createGVNPass());
			fpm->add(llvm::createPromoteMemoryToRegisterPass());
			fpm->add(llvm::createInstructionCombiningPass());
			fpm->add(llvm::createReassociatePass());
			fpm->add(llvm::createCFGSimplificationPass());
			fpm->add(llvm::createCorrelatedValuePropagationPass());
			fpm->run(*main);
		}

		//https://releases.llvm.org/8.0.0/docs/tutorial/LangImpl08.html#full-code-listing
		void complie()
		{
			//enable optimize
			
			//add_optimize();


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

		void insert_asm_locally(std::string asm_str)
		{
			llvm::InlineAsm* inlineAsm = llvm::InlineAsm::get(llvm::FunctionType::get(Type::getVoidTy(context), false), asm_str, "", false, false, llvm::InlineAsm::AD_Intel);

			builder.CreateCall(inlineAsm);
		}

		LLVMContext& context;
		IRBuilder<>& builder;
		std::unique_ptr<Module> llvm_module;
		llvm::Function* main;
		std::vector<AllocaInst*> virtual_registers;
		Value* stack; //rcx
		const vm::ctx_t& vmctx;
		
		
	};

	struct lifters
	{
		vm::handler::mnemonic_t mnemonic;  //用来定位一个vm handler对应的lifter
		std::function<void(_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t>)> hf;
	};

	lifters sregq
	{
		vm::handler::SREGQ,
		//
		//param1(context index)
		//
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1) {

		uint8_t idx = std::get<uint8_t>(param1) / 8;

		// mov x,[rcx]
		Value* dqValue = vmp2.builder.CreateLoad(vmp2.builder.getInt64Ty(), vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt64PtrTy(vmp2.context)));

		//mov vreg,x
		vmp2.builder.CreateStore(dqValue, vmp2.virtual_registers[idx]);

		//add rcx,8
		vmp2.stack = vmp2.builder.CreateAdd(vmp2.stack, vmp2.builder.getInt64(8),"new_stack");

	}
	};

	lifters sregdw
	{
		vm::handler::SREGDW,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1) {
			
		uint8_t idx = std::get<uint8_t>(param1) / 8;

		//mov     edx, [rbp+0]
		auto dwValue = vmp2.builder.CreateLoad(Type::getInt32Ty(vmp2.context),vmp2.builder.CreateIntToPtr(vmp2.stack,PointerType::getInt32PtrTy(vmp2.context)));

		//add     rbp, 4
		vmp2.stack = vmp2.builder.CreateAdd(vmp2.stack, vmp2.builder.getInt64(4));

		//mov     [rax+rdi], edx
		auto dqValue = vmp2.builder.CreateIntCast(dwValue, Type::getInt64Ty(vmp2.context), false);
		vmp2.builder.CreateStore(dqValue, vmp2.virtual_registers[idx]);

	}

	};

	lifters lregdw
	{
		vm::handler::LREGDW,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{
			uint8_t idx = std::get<uint8_t>(param1) / 8;

			//mov     edx, [rax+rdi]
			auto dqValue = vmp2.builder.CreateLoad(Type::getInt64Ty(vmp2.context),vmp2.virtual_registers[idx]);

			//i64 cast to i32
			auto dwValue = vmp2.builder.CreateIntCast(dqValue, Type::getInt32Ty(vmp2.context), false);

			//sub     rbp, 4
			vmp2.stack = vmp2.builder.CreateSub(vmp2.stack, vmp2.builder.getInt64(4));

			//mov     [rbp+0], edx
			vmp2.builder.CreateStore(dwValue, vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt32PtrTy(vmp2.context)));
			



}
	};

	lifters lregq
	{
		vm::handler::LREGQ,
		//
		//param1(context index)
		//
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1) {

		uint8_t idx = std::get<uint8_t>(param1) / 8;

		//sub rsp,8
		vmp2.stack = vmp2.builder.CreateSub(vmp2.stack, ConstantInt::get(Type::getInt64Ty(vmp2.context), APInt(64, 8)));

		//mov [rsp],[ctx]
		Value* dqValue = vmp2.builder.CreateLoad(Type::getInt64Ty(vmp2.context),vmp2.virtual_registers.at(idx));
		vmp2.builder.CreateStore(dqValue, vmp2.builder.CreateIntToPtr(vmp2.stack,PointerType::getInt64PtrTy(vmp2.context)));

	}
	};

	lifters vmexit
	{
		vm::handler::VMEXIT,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1) {

		auto iter = std::find_if(vmp2.vmctx.vm_handlers.begin(),vmp2.vmctx.vm_handlers.end(),[](vm::handler::handler_t h) {
			if (h.profile->mnemonic == vm::handler::VMEXIT)
				return true;
			else
				return false;
			});

		if (iter == vmp2.vmctx.vm_handlers.end())
			throw std::runtime_error("not have vmexit handler\n");
		
		std::string asm_str("mov rsp,rcx;");
		char buffer[256]{};
		for (auto& i : iter->instrs)
		{
			vm::util::print(i.instr, buffer);
			if (i.instr.mnemonic == ZYDIS_MNEMONIC_POP || i.instr.mnemonic == ZYDIS_MNEMONIC_POPFQ)
			{
				asm_str.append(buffer).append(";");
			}
		}


		llvm::InlineAsm* inlineAsm = llvm::InlineAsm::get(llvm::FunctionType::get(Type::getVoidTy(vmp2.context), false), asm_str, "", false, false, llvm::InlineAsm::AD_Intel);

		vmp2.builder.CreateCall(inlineAsm);

		vmp2.builder.CreateRetVoid();
		vmp2.builder.ClearInsertionPoint();

	}

	};

	lifters lconstdw
	{
		vm::handler::LCONSTDW,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{
			//sub     rbp, 4
			vmp2.stack = vmp2.builder.CreateSub(vmp2.stack, ConstantInt::get(Type::getInt64Ty(vmp2.context), APInt(64, 4)));

			//mov     [rbp+0], eax
			auto dwValue = std::get<uint32_t>(param1);
			vmp2.builder.CreateStore(vmp2.builder.getInt32(dwValue), vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt32PtrTy(vmp2.context)));

}
	};

	lifters lconstq
	{
		
		vm::handler::LCONSTQ,
		//param1 (const value to be loaded)
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{
		//sub rsp,8
		vmp2.stack = vmp2.builder.CreateSub(vmp2.stack, ConstantInt::get(Type::getInt64Ty(vmp2.context), APInt(64, 8)));

		//mov     [rbp+0], rax
		vmp2.builder.CreateStore(ConstantInt::get(Type::getInt64Ty(vmp2.context), std::get<uint64_t>(param1)), vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt64PtrTy(vmp2.context)));
			
		}
	};

	lifters lconstdwsxq
	{
		vm::handler::LCONSTDWSXQ,
		//param1 (const value to be loaded (need sign extend))
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{
			//TODO: 需要将数进行符号扩展
}
	};

	lifters readdw
	{
		vm::handler::READDW,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{
			//mov     rax, [rbp+0]
			auto readaddr = vmp2.builder.CreateLoad(Type::getInt64Ty(vmp2.context), vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt64PtrTy(vmp2.context)),"readaddr");


			//add     rbp, 4
			vmp2.stack = vmp2.builder.CreateAdd(vmp2.stack, ConstantInt::get(Type::getInt64Ty(vmp2.context), APInt(64, 4)));

			//转换成dword ptr
			auto ptr = vmp2.builder.CreateIntToPtr(readaddr, PointerType::getInt32PtrTy(vmp2.context),"readaddr_ptr");
			Value* dwValue = vmp2.builder.CreateLoad(vmp2.builder.getInt32Ty(),ptr);
		
			vmp2.builder.CreateStore(dwValue, vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt32PtrTy(vmp2.context)));

	}
	};

	lifters writedw
	{
		vm::handler::WRITEDW,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{

			//mov     rax, [rsp]
			auto writeaddr = vmp2.builder.CreateLoad(Type::getInt64Ty(vmp2.context), vmp2.builder.CreateIntToPtr(vmp2.stack, PointerType::getInt64PtrTy(vmp2.context)));

			//[rbp+8]
			auto ptr1 = vmp2.builder.CreateAdd(vmp2.stack, ConstantInt::get(Type::getInt64Ty(vmp2.context), APInt(64, 8)));
			auto dwValue = vmp2.builder.CreateLoad(Type::getInt32Ty(vmp2.context),vmp2.builder.CreateIntToPtr(ptr1, PointerType::getInt32PtrTy(vmp2.context)));

			//add rbp ,0xC
			vmp2.stack = vmp2.builder.CreateAdd(vmp2.stack, ConstantInt::get(Type::getInt64Ty(vmp2.context), APInt(64, 0xC)));

			vmp2.builder.CreateStore(dwValue, vmp2.builder.CreateIntToPtr(writeaddr, PointerType::getInt32PtrTy(vmp2.context)));


	}
	};


	lifters addq
	{
		vm::handler::ADDQ,
		[](_cvmp2& vmp2, std::variant<uint64_t, uint32_t, uint16_t, uint8_t> param1)
		{	
			//mov     rax1, [rbp+0]
			auto ptr1 = vmp2.builder.CreateIntToPtr(vmp2.stack,PointerType::getInt64PtrTy(vmp2.context));

			auto dqValue1 = vmp2.builder.CreateLoad(Type::getInt64Ty(vmp2.context),ptr1);

			//mov rax2,[rbp+8]

			auto ptr2 = vmp2.builder.CreateIntToPtr(vmp2.builder.CreateAdd(vmp2.stack, vmp2.builder.getInt64(8)), PointerType::getInt64PtrTy(vmp2.context));
			auto dqValue2 = vmp2.builder.CreateLoad(Type::getInt64Ty(vmp2.context),ptr2);

			//add rax2,rax1
			auto sum = vmp2.builder.CreateAdd(dqValue2, dqValue1);

			vmp2.builder.CreateStore(sum, ptr2);
			vmp2.builder.CreateStore(vmp2.builder.getInt64(0), ptr1);
			

		}
	};

	std::map<vm::handler::mnemonic_t, lifters> _h_map
	{
		//{vm::handler::SREGQ,sregq},
		//{vm::handler::LREGQ,lregq},
		//{vm::handler::SREGDW,sregdw},
		//{vm::handler::LREGDW,lregdw},
		//{vm::handler::LCONSTDW,lconstdw},
		//{vm::handler::LCONSTBSXDW,lconstdw},
		//{vm::handler::LCONSTWSXDW,lconstdw},
		//{vm::handler::LCONSTQ,lconstq},
		//{vm::handler::LCONSTDWSXQ,lconstq},
		//{vm::handler::READDW,readdw},
		//{vm::handler::WRITEDW,writedw},
		{vm::handler::ADDQ,addq},
		{vm::handler::VMEXIT,vmexit},
	};
	

}