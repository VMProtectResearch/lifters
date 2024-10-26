#pragma once 

#include <vmprofiles.hpp>
#include <vmctx.hpp>
#include <memory>
#include <variant>
#include <vtil/amd64>
#include <vtil/vtil>

static constexpr vtil::register_desc FLAG_CF = vtil::REG_FLAGS.select(1, 0);
static constexpr vtil::register_desc FLAG_PF = vtil::REG_FLAGS.select(1, 2);
static constexpr vtil::register_desc FLAG_AF = vtil::REG_FLAGS.select(1, 4);
static constexpr vtil::register_desc FLAG_ZF = vtil::REG_FLAGS.select(1, 6);
static constexpr vtil::register_desc FLAG_SF = vtil::REG_FLAGS.select(1, 7);
static constexpr vtil::register_desc FLAG_DF = vtil::REG_FLAGS.select(1, 10);
static constexpr vtil::register_desc FLAG_OF = vtil::REG_FLAGS.select(1, 11);

static constexpr vtil::register_desc make_virtual_register(uint8_t context_offset, uint8_t size)
{
	fassert(((context_offset & 7) + size) <= 8 && size);

	return {
		vtil::register_virtual,
		(size_t)context_offset / 8,
		size * 8,
		(context_offset % 8) * 8
	};
}

namespace lifters {
	namespace lifter_vtil {

		struct lifter_t
		{
			vm::handler::mnemonic_t mnemonic;
			std::function<void(vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk)> func;
		};

		lifter_t lconstbzxq = {
			vm::handler::LCONSTBSXQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				
				int32_t t = (int32_t)vinstr->operand.imm.u;  // 截断
				// 符号扩展?
				blk->push(vtil::operand(static_cast<int64_t>(t), 64));
			}
		};

		lifter_t lconstq = {
			// push imm<N>
			vm::handler::LCONSTQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->push(vtil::operand(vinstr->operand.imm.u, 64));
			} };

		lifter_t sregq = {
			vm::handler::SREGQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->pop(make_virtual_register(vinstr->operand.imm.u,8));
			} };

		lifter_t sregdw = {
			vm::handler::SREGDW,
			sregq.func };
		
		lifter_t sregw = {
			vm::handler::SREGW,
			sregq.func };
	
		lifter_t addq = {
			vm::handler::ADDQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64, 64);
				blk->pop(t0);
				blk->pop(t1);
				blk->add(t1, t0);
				blk->push(t1)->pushf();
			} };

		lifter_t addw = {
	vm::handler::ADDW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(32, 32);
				blk->pop(t0);
				blk->pop(t1);
				blk->add(t1, t0);
				blk->push(t1)->pushf();
			} };




		lifter_t lregq = {
			vm::handler::LREGQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto stack_top_value = blk->tmp(64);  // 栈上的值
				blk->mov(stack_top_value, make_virtual_register(vinstr->operand.imm.u, 8));  // 虚拟寄存器都是8字节
				blk->push(stack_top_value);
			}
		};
		lifter_t lregdw = {
			vm::handler::LREGDW,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto stack_top_value = blk->tmp(32);  // 栈上的值
				blk->mov(stack_top_value, make_virtual_register(vinstr->operand.imm.u, 8));
				blk->push(stack_top_value);
			}
		};

		lifter_t readq{
			vm::handler::READQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64,64);
				blk->pop(t0);
				blk->ldd(t1, t0, vtil::make_imm(0ull));
				blk->push(t1);
			}
		};
		/*
 | | 0054: [ PSEUDO ]     -0x28    strq     $sp          -0x28        0x1f
 | | 0055: [ PSEUDO ]     -0x20    lddq     t4           $sp          -0x28
 | | 0056: [ PSEUDO ]     -0x20    lddw     t3:16        t4           0x0
 | | 0057: [ PSEUDO ]     -0x22    strw     $sp          -0x22        t3:16
		*/
		lifter_t readw{
			vm::handler::READW,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64,16);
				blk->pop(t0); // 拿出地址
				blk->ldd(t1, t0, vtil::make_imm(0ull));
				blk->push(t1);
			}
		};

		lifter_t nandq{
			vm::handler::NANDQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64, 64);
				blk->pop(t0);  // mov     rax, [rbp+0]  
				blk->pop(t1);  // mov     rdx, [rbp+8]
				blk->bnot(t1); // not rdx
				blk->bnot(t0); // not rax
				blk->band(t0, t1); // and rax,rdx
				blk->push(t0);
				blk->pushf();

			}
		};

		lifter_t nanddw{
		vm::handler::NANDDW,
		};

		lifter_t nandw{
			vm::handler::NANDW,
		};		
		

		lifter_t writeq{
			vm::handler::WRITEQ,
		};

		lifter_t writew{
			vm::handler::WRITEW,
		};
		
		
		lifter_t shrq{
			vm::handler::SHRQ,
		};


		lifter_t shrw{
			vm::handler::SHRW,
		};

		lifter_t shrb{
			vm::handler::SHRB,
		};

		/*
 | | 0054: [ PSEUDO ]     -0x20    movq     t3           $sp
 | | 0055: [ PSEUDO ]     -0x28    strq     $sp          -0x28        t3
		*/
		lifter_t pushvspq{
			vm::handler::PUSHVSPQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->push(vtil::REG_SP);
		}
		};


	lifter_t LiftersArray[] = {
		lconstbzxq,lconstq,
		sregq,
		addq,
		lregq,
		pushvspq,
		readq,readw,
		nandq,
	};



		};
}
