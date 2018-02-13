#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerFSqrt::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	int width = op1.size;

	assert(op1.is_vreg());
	assert(dest.is_vreg());

	assert(op1.is_alloc_reg());

	assert(op1.size == dest.size);
	assert(op1.size == 4 || op1.size == 8);

	const auto &op1_reg = op1.is_alloc_reg() ? GetCompiler().register_from_operand(&op1) : BLKJIT_TEMPS_0(8);

	// dest = op1 - op2
	if(width == 4) {
		Encoder().movq(op1_reg, BLKJIT_FP_0);

		// dest = dest - source
		// dest = fp_1
		// source = fp_0
		Encoder().sqrtss(BLKJIT_FP_0, BLKJIT_FP_1);

		Encoder().movq(BLKJIT_FP_1, GetCompiler().register_from_operand(&dest));
	} else if(width == 8) {
		Encoder().movq(op1_reg, BLKJIT_FP_0);

		Encoder().sqrtsd(BLKJIT_FP_0, BLKJIT_FP_1);

		Encoder().movq(BLKJIT_FP_1, GetCompiler().register_from_operand(&dest));
	}

	insn++;

	return true;
}