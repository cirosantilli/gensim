#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"
#include "translate/jit_funs.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerFCtrl_GetRound::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *dest = &insn->operands[0];

	GetCompiler().emit_save_reg_state(1, GetStackMap(), GetIsStackFixed());

	GetCompiler().load_state_field(0, REG_RDI);

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&cpuGetRoundingMode, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	Encoder().mov(REG_RAX, BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(1, GetStackMap(), GetIsStackFixed());

	// Pop the reference argument value into the destination register
	if (dest->is_alloc_reg()) {
		Encoder().mov(BLKJIT_RETURN(dest->size), GetCompiler().register_from_operand(dest));
	} else {
		assert(false);
	}

	insn++;
	return true;
}

bool LowerFCtrl_SetRound::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *mode = &insn->operands[0];

	GetCompiler().emit_save_reg_state(2, GetStackMap(), GetIsStackFixed());

	GetCompiler().load_state_field(0, REG_RDI);

	GetCompiler().encode_operand_function_argument(mode, REG_RSI, GetStackMap());

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&cpuSetRoundingMode, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(2, GetStackMap(), GetIsStackFixed());

	insn++;
	return true;
}

bool LowerFCtrl_GetFlush::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *dest = &insn->operands[0];

	GetCompiler().emit_save_reg_state(1, GetStackMap(), GetIsStackFixed());

	GetCompiler().load_state_field(0, REG_RDI);

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&cpuGetFlushMode, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	Encoder().mov(REG_RAX, BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(1, GetStackMap(), GetIsStackFixed());

	// Pop the reference argument value into the destination register
	if (dest->is_alloc_reg()) {
		Encoder().mov(BLKJIT_RETURN(dest->size), GetCompiler().register_from_operand(dest));
	} else {
		assert(false);
	}

	insn++;
	return true;
}

bool LowerFCtrl_SetFlush::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *mode = &insn->operands[0];

	// Load the address of the stack slot into RCX
	Encoder().mov(REG_RSP, REG_RCX);

	GetCompiler().emit_save_reg_state(2, GetStackMap(), GetIsStackFixed());

	GetCompiler().load_state_field(0, REG_RDI);

	GetCompiler().encode_operand_function_argument(mode, REG_RSI, GetStackMap());

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&cpuSetFlushMode, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(2, GetStackMap(), GetIsStackFixed());

	insn++;
	return true;
}