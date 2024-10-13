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


		lifter_t lconstq = {
			// push imm<N>
			vm::handler::LCONSTQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->push(vtil::operand(vinstr->operand.imm.u, 64));
			} };


		lifter_t sregq = {
			vm::handler::SREGQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
			} };
	
		lifter_t addq = {
			vm::handler::ADDQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1, t2] = blk->tmp(64, 64, 64);
				auto [b0, b1, b2, b3] = blk->tmp(1, 1, 1, 1);
				blk->pop(t0);
				blk->pop(t1);
				blk->mov(t2, t1);
				blk->add(t1, t0);
				blk->tl(FLAG_SF, t1, 0)->
					te(FLAG_ZF, t1, 0)->
					tul(FLAG_CF, t1, t2)->
					tl(b2, t2, 0)->
					tl(b3, t0, 0)->
					te(b0, b2, b3)->
					tl(b2, t2, 0)->
					tl(b3, t1, 0)->
					tne(b1, b2, b3)->
					mov(FLAG_OF, b0)->
					band(FLAG_OF, b1);
			} };

	lifter_t LiftersArray[] = {
		lconstq,sregq,addq
	};



		};
}
