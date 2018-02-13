/*
 * LowerMemoryGeneric.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"
#include "translate/jit_funs.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerReadMemGeneric::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *offset = &insn->operands[0];
	const IROperand *disp = &insn->operands[1];
	const IROperand *dest = &insn->operands[2];

	assert(offset->is_alloc_reg());
	assert(disp->is_constant());

	// liveness is broken in some situations apparently
	uint32_t live_regs = 0xffffffff;//host_liveness.live_out[ir_idx];
	// Don't save/restore the destination reg
	if(dest->is_alloc_reg()) live_regs &= ~(1 << dest->alloc_data);

	GetCompiler().emit_save_reg_state(2, GetStackMap(), GetIsStackFixed(), live_regs);

	//TODO: support these things coming in on the stack
	assert(disp->is_constant());
	assert(offset->is_alloc_reg());
	assert(!dest->is_alloc_stack());

	// We can have the situation where dest is not allocated because the intervening register write has been eliminated
	const auto &dest_reg = dest->is_allocated() ? GetCompiler().get_allocable_register(dest->alloc_data, dest->size) : GetCompiler().get_temp(0, dest->size);

	Encoder().mov(BLKJIT_CPUSTATE_REG, BLKJIT_ARG0(8));
	if(disp->value != 0) {
		Encoder().lea(X86Memory::get(GetCompiler().register_from_operand(offset, 4), disp->value), BLKJIT_ARG1(4));
	} else {
		Encoder().mov(GetCompiler().register_from_operand(offset, 4), BLKJIT_ARG1(4));
	}

	// Call the appropriate blockjit-CC function
	switch(dest->size) {
		case 1:
			Encoder().call((void*)blkRead8, BLKJIT_RETURN(8));
			break;
		case 2:
			Encoder().call((void*)blkRead16, BLKJIT_RETURN(8));
			break;
		case 4:
			Encoder().call((void*)blkRead32, BLKJIT_RETURN(8));
			break;
	}
	if(dest->is_alloc_reg()) {
		Encoder().mov(REGS_RAX(dest->size), GetCompiler().register_from_operand(dest, dest->size));
	}
	GetCompiler().emit_restore_reg_state(2, GetStackMap(), GetIsStackFixed(), live_regs);

	insn++;
	return true;
}

bool LowerWriteMemGeneric::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *value = &insn->operands[0];
	const IROperand *disp = &insn->operands[1];
	const IROperand *offset = &insn->operands[2];

	assert(disp->is_constant());

	assert(offset->is_alloc_reg());
	assert(disp->is_constant());

	if(value->is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(value), BLKJIT_ARG2(4));
	}

	GetCompiler().emit_save_reg_state(3, GetStackMap(), GetIsStackFixed());

	//State register will be dealt with by the shunt
	auto &address_reg = BLKJIT_ARG1(4);

	GetCompiler().load_state_field(0, BLKJIT_ARG0(8));
	if(offset->is_alloc_reg()) {
		Encoder().mov(GetCompiler().get_allocable_register(offset->alloc_data, 4), address_reg);
	} else {
		assert(false);
	}
	if(disp->is_alloc_reg()) {
		Encoder().add(GetCompiler().get_allocable_register(disp->alloc_data, 4), address_reg);
	} else if(disp->is_constant()) {
		if(disp->value != 0)
			Encoder().add(disp->value, address_reg);
	} else {
		assert(false);
	}

	//finally, value
	if(value->is_alloc_reg()) {
		Encoder().mov(GetCompiler().get_allocable_register(value->alloc_data, 4), BLKJIT_ARG2(4));
	} else if(value->is_alloc_stack()) {
		// already loaded
	} else if(value->is_constant()) {
		if(value->size != 4) {
			assert(false);
		}
		Encoder().mov(value->value, BLKJIT_ARG2(4));
	} else {
		assert(false);
	}

	switch(value->size) {
		case 1:
			Encoder().mov((uint64_t)cpuWrite8, BLKJIT_RETURN(8));
			break;
		case 2:
			Encoder().mov((uint64_t)cpuWrite16, BLKJIT_RETURN(8));
			break;
		case 4:
			Encoder().mov((uint64_t)cpuWrite32, BLKJIT_RETURN(8));
			break;
	}

	Encoder().call(BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(3, GetStackMap(), GetIsStackFixed());

	insn++;
	return true;
}

bool LowerReadUserMemGeneric::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *offset = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	GetCompiler().emit_save_reg_state(3, GetStackMap(), GetIsStackFixed());

	switch(dest->size) {
		case 1:
			Encoder().mov((uint64_t)cpuRead8User, BLKJIT_RETURN(8));
			break;
		case 2:
			assert(false);
			break;
		case 4:
			Encoder().mov((uint64_t)cpuRead32User, BLKJIT_RETURN(8));
			break;
		default:
			assert(false);
	}

	GetCompiler().load_state_field(0, REG_RDI);
	GetCompiler().encode_operand_function_argument(offset, REG_RSI, GetStackMap());

	Encoder().push(0);
	Encoder().mov(REG_RSP, REG_RDX);

	Encoder().call(BLKJIT_RETURN(8));
	Encoder().pop(GetCompiler().get_temp(0, 8));

	GetCompiler().emit_restore_reg_state(3, GetStackMap(), GetIsStackFixed());

	if(dest->is_alloc_reg()) {
		Encoder().xorr(GetCompiler().register_from_operand(dest),GetCompiler().register_from_operand(dest));
		Encoder().mov(GetCompiler().get_temp(0, dest->size), GetCompiler().register_from_operand(dest));
	} else if(dest->is_alloc_stack()) {
		Encoder().mov(GetCompiler().get_temp(0, dest->size), GetCompiler().stack_from_operand(dest));
	}

	insn++;
	return true;
}

bool LowerWriteUserMemGeneric::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *value = &insn->operands[0];
	const IROperand *offset = &insn->operands[1];

	GetCompiler().emit_save_reg_state(3, GetStackMap(), GetIsStackFixed());

	switch(value->size) {
		case 1:
			Encoder().mov((uint64_t)cpuWrite8User, BLKJIT_RETURN(8));
			break;
		case 2:
			assert(false);
			break;
		case 4:
			Encoder().mov((uint64_t)cpuWrite32User, BLKJIT_RETURN(8));
			break;
		default:
			assert(false);
	}

	GetCompiler().load_state_field(0, REG_RDI);
	GetCompiler().encode_operand_function_argument(offset, REG_RSI, GetStackMap());
	GetCompiler().encode_operand_function_argument(value, REG_RDX, GetStackMap());

	Encoder().call(BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(3, GetStackMap(), GetIsStackFixed());

	insn++;
	return true;
}