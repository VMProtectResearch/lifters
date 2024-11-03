#pragma once

#include <memory>
#include <variant>
#include <vmctx.hpp>
#include <vmprofiles.hpp>
#include <vtil/amd64>
#include <vtil/vtil>

static constexpr vtil::register_desc FLAG_CF = vtil::REG_FLAGS.select( 1, 0 );
static constexpr vtil::register_desc FLAG_PF = vtil::REG_FLAGS.select( 1, 2 );
static constexpr vtil::register_desc FLAG_AF = vtil::REG_FLAGS.select( 1, 4 );
static constexpr vtil::register_desc FLAG_ZF = vtil::REG_FLAGS.select( 1, 6 );
static constexpr vtil::register_desc FLAG_SF = vtil::REG_FLAGS.select( 1, 7 );
static constexpr vtil::register_desc FLAG_DF = vtil::REG_FLAGS.select( 1, 10 );
static constexpr vtil::register_desc FLAG_OF = vtil::REG_FLAGS.select( 1, 11 );

static constexpr vtil::register_desc make_virtual_register( uint8_t context_offset, uint8_t size )
{
    fassert( ( ( context_offset & 7 ) + size ) <= 8 && size );

    return { vtil::register_virtual, ( size_t )context_offset / 8, size * 8, ( context_offset % 8 ) * 8 };
}

namespace lifters
{
    namespace lifter_vtil
    {

        struct lifter_t
        {
            vm::handler::mnemonic_t mnemonic;
            std::function< void( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                 vmp2::v3::code_block_t *code_blk ) >
                func;
        };

        lifter_t lconstbzxq = { vm::handler::LCONSTBSXQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                             vmp2::v3::code_block_t *code_blk )
                                {
                                    int32_t t = ( int32_t )vinstr->operand.imm.u; // 截断
                                    // 符号扩展?
                                    blk->push( vtil::operand( static_cast< int64_t >( t ), 64 ) );
                                } };

        lifter_t lconstq = {
            // push imm<N>
            vm::handler::LCONSTQ,
            []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
            { blk->push( vtil::operand( vinstr->operand.imm.u, 64 ) ); } };

        lifter_t lconstdw = {
            // push imm<N>
            vm::handler::LCONSTDW,
            []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
            { blk->push( vtil::operand( vinstr->operand.imm.u, 32 ) ); } };

        lifter_t lconstw = {
            // push imm<N>
            vm::handler::LCONSTW,
            []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
            { blk->push( vtil::operand( vinstr->operand.imm.u, 16 ) ); } };

        lifter_t lconstb2w = {
            // push imm<N>
            vm::handler::LCONSTB2W,
            []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
            { blk->push( vtil::operand( vinstr->operand.imm.u, 16 ) ); } };

        lifter_t lconstbzxw{ vm::handler::LCONSTBZXW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                          vmp2::v3::code_block_t *code_blk )
                             { blk->push( vtil::operand( vinstr->operand.imm.u, 16 ) ); } };

        lifter_t lconstwsxq{ vm::handler::LCONSTWSXQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                          vmp2::v3::code_block_t *code_blk )
                             { blk->push( vtil::operand( vinstr->operand.imm.u, 64 ) ); } };

        lifter_t sregq = { vm::handler::SREGQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                   vmp2::v3::code_block_t *code_blk )
                           { blk->pop( make_virtual_register( vinstr->operand.imm.u, 8 ) ); } };

        lifter_t sregdw = { vm::handler::SREGDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                     vmp2::v3::code_block_t *code_blk )
                            { blk->pop( make_virtual_register( vinstr->operand.imm.u, 4 ) ); } };

        lifter_t sregw = { vm::handler::SREGW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                   vmp2::v3::code_block_t *code_blk )
                           {
                               // mov     dx, [rbp+0]
                               // mov     [rax+rdi], dx
                               blk->pop( make_virtual_register( vinstr->operand.imm.u, 2 ) );
                           } };

        lifter_t sregb = { vm::handler::SREGB, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                   vmp2::v3::code_block_t *code_blk )
                           { blk->pop( make_virtual_register( vinstr->operand.imm.u, 1 ) ); } };

        lifter_t addq = { vm::handler::ADDQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                 vmp2::v3::code_block_t *code_blk )
                          {
                              auto [ t0, t1 ] = blk->tmp( 64, 64 );
                              blk->pop( t0 );
                              blk->pop( t1 );
                              blk->add( t1, t0 );
                              blk->push( t1 )->pushf();
                          } };

        lifter_t addw = { vm::handler::ADDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                 vmp2::v3::code_block_t *code_blk )
                          {
                              auto [ t0, t1 ] = blk->tmp( 16, 16 );
                              blk->pop( t0 );
                              blk->pop( t1 );
                              blk->add( t1, t0 );
                              blk->push( t1 )->pushf();
                          } };

        lifter_t adddw = { vm::handler::ADDDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                   vmp2::v3::code_block_t *code_blk )
                           {
                               auto [ t0, t1 ] = blk->tmp( 32, 32 );
                               blk->pop( t0 );
                               blk->pop( t1 );
                               blk->add( t1, t0 );
                               blk->push( t1 )->pushf();
                           } };

        lifter_t addb = { vm::handler::ADDB, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                 vmp2::v3::code_block_t *code_blk )
                          {
                              auto [ t0, t1 ] = blk->tmp( 8, 8 );
                              blk->pop( t0 );
                              blk->pop( t1 );
                              blk->add( t1, t0 );
                              blk->push( t1 )->pushf();
                          } };

        lifter_t lregq = {
            vm::handler::LREGQ,
            []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
            {
                auto stack_top_value = blk->tmp( 64 );                                          // 栈上的值
                blk->mov( stack_top_value, make_virtual_register( vinstr->operand.imm.u, 8 ) ); // 虚拟寄存器都是8字节
                blk->push( stack_top_value );
            } };
        lifter_t lregdw = { vm::handler::LREGDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                     vmp2::v3::code_block_t *code_blk )
                            {
                                auto stack_top_value = blk->tmp( 32 ); // 栈上的值
                                blk->mov( stack_top_value, make_virtual_register( vinstr->operand.imm.u, 4 ) );
                                blk->push( stack_top_value );
                            } };

        lifter_t lregw = { vm::handler::LREGW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                   vmp2::v3::code_block_t *code_blk )
                           {
                               auto stack_top_value = blk->tmp( 16 ); // 栈上的值
                               blk->mov( stack_top_value, make_virtual_register( vinstr->operand.imm.u, 2 ) );
                               blk->push( stack_top_value );
                           } };

        lifter_t readq{ vm::handler::READQ,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            auto [ t0, t1 ] = blk->tmp( 64, 64 );
                            blk->pop( t0 );
                            blk->ldd( t1, t0, vtil::make_imm( 0ull ) );
                            blk->push( t1 );
                        } };

        lifter_t readdw{ vm::handler::READDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                  vmp2::v3::code_block_t *code_blk )
                         {
                             auto [ t0, t1 ] = blk->tmp( 64, 32 );
                             blk->pop( t0 );
                             blk->ldd( t1, t0, vtil::make_imm( 0ull ) );
                             blk->push( t1 );
                         } };

        lifter_t readw{ vm::handler::READW,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            auto [ t0, t1 ] = blk->tmp( 64, 16 );
                            blk->pop( t0 ); // 拿出地址
                            blk->ldd( t1, t0, vtil::make_imm( 0ull ) );
                            blk->push( t1 );
                        } };

        lifter_t readb{ vm::handler::READB,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            auto [ t0, t1 ] = blk->tmp( 64, 8 );
                            blk->pop( t0 ); // 拿出地址
                            blk->ldd( t1, t0, vtil::make_imm( 0ull ) );
                            blk->push( t1 );
                        } };

        lifter_t nandq{ vm::handler::NANDQ,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            auto [ t0, t1 ] = blk->tmp( 64, 64 );
                            blk->pop( t0 );      // mov     rax, [rbp+0]
                            blk->pop( t1 );      // mov     rdx, [rbp+8]
                            blk->bnot( t1 );     // not rdx
                            blk->bnot( t0 );     // not rax
                            blk->band( t0, t1 ); // and rax,rdx
                            blk->push( t0 );
                            blk->pushf();
                        } };

        lifter_t nanddw{ vm::handler::NANDDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                  vmp2::v3::code_block_t *code_blk )
                         {
                             auto [ t0, t1 ] = blk->tmp( 32, 32 );
                             blk->pop( t0 );
                             blk->pop( t1 );
                             blk->bnot( t1 );
                             blk->bnot( t0 );
                             blk->band( t0, t1 );
                             blk->push( t0 );
                             blk->pushf();
                         } };

        lifter_t nandw{ vm::handler::NANDW,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            auto [ t0, t1 ] = blk->tmp( 16, 16 );
                            blk->pop( t0 );
                            blk->pop( t1 );
                            blk->bnot( t1 );
                            blk->bnot( t0 );
                            blk->band( t0, t1 );
                            blk->push( t0 );
                            blk->pushf();
                        } };

        lifter_t nandb{ vm::handler::NANDB,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            auto [ t0, t1 ] = blk->tmp( 8, 8 );
                            blk->pop( t0 );
                            blk->pop( t1 );
                            blk->bnot( t1 );
                            blk->bnot( t0 );
                            blk->band( t0, t1 );
                            blk->push( t0 );
                            blk->pushf();
                        } };

        lifter_t writeq{ vm::handler::WRITEQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                  vmp2::v3::code_block_t *code_blk )
                         {
                             auto [ t0, t1 ] = blk->tmp( 64, 64 );
                             blk->pop( t0 );                             // mov     rax, [rbp+0]
                             blk->pop( t1 );                             // mov     rdx, [rbp+8]
                             blk->str( t0, vtil::make_imm( 0ull ), t1 ); // mov     [rax], rdx
                         } };

        lifter_t writew{ vm::handler::WRITEW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                  vmp2::v3::code_block_t *code_blk )
                         {
                             auto [ t0, t1 ] = blk->tmp( 64, 16 );
                             blk->pop( t0 );                             // mov     rax, [rbp+0]
                             blk->pop( t1 );                             // mov     dx, [rbp+8]
                             blk->str( t0, vtil::make_imm( 0ull ), t1 ); // mov     [rax], rdx
                         } };

        lifter_t writedw{ vm::handler::WRITEDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                    vmp2::v3::code_block_t *code_blk )
                          {
                              auto [ t0, t1 ] = blk->tmp( 64, 32 );
                              blk->pop( t0 );                             // mov     rax, [rbp+0]
                              blk->pop( t1 );                             // mov     edx, [rbp+8]
                              blk->str( t0, vtil::make_imm( 0ull ), t1 ); // mov     [rax], rdx
                          } };

        lifter_t writeb{ vm::handler::WRITEB, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                  vmp2::v3::code_block_t *code_blk )
                         {
                             auto [ t0, t1 ] = blk->tmp( 64, 8 );
                             blk->pop( t0 );
                             blk->pop( t1 );
                             blk->str( t0, vtil::make_imm( 0ull ), t1 );
                         } };

        lifter_t shrq{ vm::handler::SHRQ,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           //
                           auto [ t0, t1 ] = blk->tmp( 64, 64 );
                           blk->pop( t0 );
                           blk->pop( t1 );
                           blk->bshr( t0, t1 );
                           blk->push( t0 );
                           blk->pushf();
                       } };

        lifter_t shrw{ vm::handler::SHRW,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           //
                           auto [ t0, t1 ] = blk->tmp( 16, 16 );
                           // mov     ax, [rbp+0]
                           blk->pop( t0 );
                           // mov     cl, [rbp+2]
                           blk->pop( t1 );
                           // shr     ax, cl
                           blk->bshr( t0, t1 );
                           blk->push( t0 );
                           blk->pushf();
                       } };

        lifter_t shrb{ vm::handler::SHRB,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           //
                           auto [ t0, t1 ] = blk->tmp( 8, 8 );
                           blk->pop( t0 );
                           blk->pop( t1 );
                           blk->bshr( t0, t1 );
                           blk->push( t0 );
                           blk->pushf();
                       } };

        lifter_t shlb{ vm::handler::SHLB,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           //
                           auto [ t0, t1 ] = blk->tmp( 8, 8 );
                           blk->pop( t0 );
                           blk->pop( t1 );
                           blk->bshr( t0, t1 );
                           blk->push( t0 );
                           blk->pushf();
                       } };

        lifter_t shldw{ vm::handler::SHLDW,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            //
                            auto [ t0, t1 ] = blk->tmp( 32, 32 );
                            blk->pop( t0 );
                            blk->pop( t1 );
                            blk->bshr( t0, t1 );
                            blk->push( t0 );
                            blk->pushf();
                        } };

        lifter_t shlq{ vm::handler::SHLQ,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           //
                           auto [ t0, t1 ] = blk->tmp( 64, 64 );
                           blk->pop( t0 );
                           blk->pop( t1 );
                           blk->bshr( t0, t1 );
                           blk->push( t0 );
                           blk->pushf();
                       } };

        lifter_t pushvspq{ vm::handler::PUSHVSPQ,
                           []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                               vmp2::v3::code_block_t *code_blk ) { blk->push( vtil::REG_SP ); } };

        lifter_t popvspq{ vm::handler::POPVSPQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                    vmp2::v3::code_block_t *code_blk ) { blk->pop( vtil::REG_SP ); } };

        lifter_t lflagsq{ vm::handler::LFLAGSQ, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                    vmp2::v3::code_block_t *code_blk ) { blk->popf(); } };

        lifter_t rdtsc{ vm::handler::RDTSC,
                        []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                        {
                            blk->vemits( "rdtsc" )
                                ->vpinw( X86_REG_RDX )
                                ->vpinw( X86_REG_RAX )

                                // [rsp + 4] := edx
                                // [rsp] := eax
                                ->push( X86_REG_EAX )
                                ->push( X86_REG_EDX );
                        } };

        lifter_t divw{ vm::handler::DIVW,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           auto [ a0, a1, d, c ] = blk->tmp( 16, 16, 16, 16 );
                           blk
                               // t0 := [rsp]
                               // t1 := [rsp+*]
                               // t2 := [rsp+2*]
                               ->pop( d )  // d
                               ->pop( a0 ) // a
                               ->pop( c )  // c
                               ->mov( a1, a0 )

                               // div
                               ->div( a0, d, c )
                               ->rem( a1, d, c )

                               // [rsp] := flags
                               // [rsp+8] := t0
                               // [rsp+8+*] := t1
                               ->push( a0 )
                               ->push( a1 )
                               ->pushf();
                       } };

        lifter_t mulw{ vm::handler::MULW,
                       []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk )
                       {
                           auto [ a0, a1, d ] = blk->tmp( 16, 16, 16 );

                           blk
                               // t0 := [rsp]
                               // t1 := [rsp+*]
                               ->pop( d )  // d
                               ->pop( a0 ) // a
                               ->mov( a1, a0 )

                               // mul
                               ->mul( a0, d )
                               ->mulhi( a1, d )
                               //->upflg( vtil::REG_FLAGS ) TODO

                               // [rsp] := flags
                               // [rsp+8] := t0
                               // [rsp+8+*] := t1
                               ->push( a0 )
                               ->push( a1 )
                               ->pushf();
                       } };

        lifter_t shlddw = { vm::handler::SHLDDW, []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr,
                                                     vmp2::v3::code_block_t *code_blk )
                            {
                                auto [ t0, t1, t2, t3 ] = blk->tmp( 32, 32, 32, 32 );

                                blk
                                    // t0 := [rsp]
                                    // t1 := [rsp+*]
                                    // t2 := [rsp+2*]
                                    ->pop( t0 )
                                    ->pop( t1 )
                                    ->pop( t2 )

                                    // t0 := t0 >> t2
                                    ->bshl( t0, t2 )

                                    // t3 := v[0]*8
                                    // t3 -= t2
                                    ->mov( t3, vtil::make_imm< uint8_t >( 32 ) )
                                    ->sub( t3, t2 )

                                    // t1 := t1 << t3
                                    ->bshr( t1, t3 )

                                    // t0 |= t1
                                    ->bor( t0, t1 )
                                    //->upflg( vtil::REG_FLAGS ) TODO

                                    // [rsp+8] := t0
                                    ->push( t0 )
                                    ->pushf();
                            } };

        lifter_t LiftersArray[] = {
            lconstbzxq, lconstq, lconstdw, lconstbzxw, lconstwsxq, lconstw, lconstb2w, sregq,    sregw,   sregdw, sregb,
            addq,       adddw,   addw,     addb,       lregq,      lregdw,  lregw,     pushvspq, popvspq, readq,  readw,
            readdw,     readb,   writeq,   writedw,    writew,     writeb,  nandq,     nanddw,   nandw,   nandb,  shrq,
            shrw,       shrb,    shlq,     shldw,      shlb,       shlddw,  lflagsq,   rdtsc,    divw,    mulw,
        };

    }; // namespace lifter_vtil
} // namespace lifters