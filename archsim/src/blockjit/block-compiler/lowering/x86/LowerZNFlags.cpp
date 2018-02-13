/*
 * LowerZNFlags.cpp
 *
 *  Created on: 1 Dec 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

#include "gensim/gensim_processor_blockjit.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerZNFlags::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand &value = insn->operands[0];

	if(value.is_alloc_reg()) {
		auto &input_reg = GetCompiler().register_from_operand(&value);
		Encoder().test(input_reg, input_reg);

		const gensim::BlockJitProcessor *cpu = GetCompiler().get_cpu();
		uint32_t z_o = cpu->GetRegisterDescriptor("Z").Offset;
		uint32_t n_o = cpu->GetRegisterDescriptor("N").Offset;

		Encoder().setz(X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
		Encoder().sets(X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
	} else if(value.is_alloc_stack()) {
		assert(false);
	} else if(value.is_constant()) {
		// generate instructions depending on fixed value
		// 3 different conditions (since value cannot be signed and zero)
		int64_t v = value.value;
		const gensim::BlockJitProcessor *cpu = GetCompiler().get_cpu();
		uint32_t z_o = cpu->GetRegisterDescriptor("Z").Offset;
		uint32_t n_o = cpu->GetRegisterDescriptor("N").Offset;

		if(v == 0) {
			// clear n, set Z
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
			Encoder().mov1(1, X86Memory::get(BLKJIT_REGSTATE_REG, z_o));

		} else if(v < 0) {
			// set n, clear Z
			Encoder().mov1(1, X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
		} else {
			// clear n and z
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
		}

	} else {
		assert(false);
	}

	insn++;
	return true;
}

