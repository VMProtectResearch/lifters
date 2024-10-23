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

		lifter_t lconstdw = {
			// push imm<N>
			vm::handler::LCONSTDW,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->push(vtil::operand(vinstr->operand.imm.u, 32));
			} };

		lifter_t lconstbzxw{
	vm::handler::LCONSTBZXW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->push(vtil::operand(vinstr->operand.imm.u, 16));
}
		};

		lifter_t sregq = {
			vm::handler::SREGQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
			} };

		lifter_t sregw = {
		vm::handler::SREGW,
		[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
			blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
		} };

		lifter_t sregdw = {
	vm::handler::SREGDW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
	} };

		lifter_t sregb = {
vm::handler::SREGB,
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
					band(FLAG_OF, b1)->push(t1)->pushf();
			} };

		lifter_t addw = {
	vm::handler::ADDW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1, t2] = blk->tmp(16, 16, 16);
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
			band(FLAG_OF, b1)->push(t1)->pushf();
	} };


		lifter_t lregq = {
			vm::handler::LREGQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
			}
		};
		lifter_t lregdw = {
	vm::handler::LREGDW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
	}
		};
		lifter_t lregw = {
			vm::handler::LREGW,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				blk->pop(make_virtual_register(vinstr->operand.imm.u,vinstr->operand.imm.imm_size));
			}
		};

		lifter_t readw{
			vm::handler::READW,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64,16);
				blk->pop(t0);
				blk->ldd(t1, t0, vtil::make_imm(0ull));
				blk->push(t1);
			}
		};

		lifter_t readb{
	vm::handler::READB,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1] = blk->tmp(64,8);
		blk->pop(t0);
		blk->ldd(t1, t0, vtil::make_imm(0ull));
		blk->push(t1);
	}
		};

		lifter_t readdw{
		vm::handler::READDW,
		[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
			auto [t0, t1] = blk->tmp(64,32);
			blk->pop(t0);
			blk->ldd(t1, t0, vtil::make_imm(0ull));
			blk->push(t1);
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

		lifter_t nandq{
			vm::handler::NANDQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64, 64);
				blk->pop(t0);
				blk->pop(t1);
				blk->bnot(t0);
				blk->bnot(t1);
				blk->bor(t0, t1);
				blk->tl(FLAG_SF, t0, 0);
				blk->te(FLAG_ZF, t0, 0);
				blk->mov(FLAG_OF, 0);
				blk->mov(FLAG_CF, 0);
				blk->push(t0);
				blk->pushf();

			}
		};

		lifter_t nandw{
	vm::handler::NANDW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1] = blk->tmp(16, 16);
		blk->pop(t0);
		blk->pop(t1);
		blk->bnot(t0);
		blk->bnot(t1);
		blk->bor(t0, t1);
		blk->tl(FLAG_SF, t0, 0);
		blk->te(FLAG_ZF, t0, 0);
		blk->mov(FLAG_OF, 0);
		blk->mov(FLAG_CF, 0);
		blk->push(t0);
		blk->pushf();

	}
		};		lifter_t nanddw{
	vm::handler::NANDDW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1] = blk->tmp(32, 32);
		blk->pop(t0);
		blk->pop(t1);
		blk->bnot(t0);
		blk->bnot(t1);
		blk->bor(t0, t1);
		blk->tl(FLAG_SF, t0, 0);
		blk->te(FLAG_ZF, t0, 0);
		blk->mov(FLAG_OF, 0);
		blk->mov(FLAG_CF, 0);
		blk->push(t0);
		blk->pushf();

	}
		};

		lifter_t writeq{
			vm::handler::WRITEQ,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1] = blk->tmp(64, 64);
				blk->pop(t0);
				blk->pop(t1);
				blk->str(t0, vtil::make_imm(0ull), t1);
			}
		};

		lifter_t writew{
	vm::handler::WRITEW,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1] = blk->tmp(64, 32);
		blk->pop(t0);
		blk->pop(t1);
		blk->str(t0, vtil::make_imm(0ull), t1);
	}
		};
		lifter_t shrq{
	vm::handler::SHRQ,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1, t2] = blk->tmp(64, 64, 8);
		auto cf = t1;
		cf.bit_offset = cf.bit_count - 1; cf.bit_count = 1;
		cf.bit_count = 1;
		auto ofx = t0;
		ofx.bit_offset = ofx.bit_count - 1;
		ofx.bit_count = 1;
		blk->pop(t0)->pop(t2)->mov(t1, t0)->bshr(t0, t2)->tl(FLAG_SF, t0, 0)->te(FLAG_ZF, t0, 0)->mov(FLAG_OF, ofx)->mov(FLAG_CF, cf)
			->bxor(FLAG_OF, cf)->push(t0)->pushf();
	}
		};
		lifter_t shrw{
			vm::handler::SHRW,
			[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
				auto [t0, t1, t2] = blk->tmp(16, 16, 8);
				auto cf = t1;
				cf.bit_offset = cf.bit_count - 1; cf.bit_count = 1;
				cf.bit_count = 1;
				auto ofx = t0;
				ofx.bit_offset = ofx.bit_count - 1;
				ofx.bit_count = 1;
				blk->pop(t0)->pop(t2)->mov(t1, t0)->bshr(t0, t2)->tl(FLAG_SF, t0, 0)->te(FLAG_ZF, t0, 0)->mov(FLAG_OF, ofx)->mov(FLAG_CF, cf)
					->bxor(FLAG_OF, cf)->push(t0)->pushf();
			}
		};

		lifter_t shrb{
	vm::handler::SHRB,
	[](vtil::basic_block* blk, vm::instrs::virt_instr_t* vinstr, vmp2::v3::code_block_t* code_blk) {
		auto [t0, t1, t2] = blk->tmp(8, 8, 8);
		auto cf = t1;
		cf.bit_offset = cf.bit_count - 1; cf.bit_count = 1;
		cf.bit_count = 1;
		auto ofx = t0;
		ofx.bit_offset = ofx.bit_count - 1;
		ofx.bit_count = 1;
		blk->pop(t0)->pop(t2)->mov(t1, t0)->bshr(t0, t2)->tl(FLAG_SF, t0, 0)->te(FLAG_ZF, t0, 0)->mov(FLAG_OF, ofx)->mov(FLAG_CF, cf)
			->bxor(FLAG_OF, cf)->push(t0)->pushf();
	}
		};


	lifter_t LiftersArray[] = {
		lconstq,lconstbzxw,lconstdw,
		sregq,sregw,sregdw,sregb,
		addq,addw,
		lregq,lregw,lregdw,
		readw,readb,readdw,readq,
		nandq,nandw,nanddw,
		writeq,writew,
		shrw,shrb,shrq,
	};



		};
}
