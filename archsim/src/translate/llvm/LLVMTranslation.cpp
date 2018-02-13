#include "translate/llvm/LLVMTranslation.h"
#include "translate/llvm/LLVMMemoryManager.h"
#include "gensim/gensim_processor.h"


#include <llvm/ExecutionEngine/ExecutionEngine.h>

using namespace archsim::translate;
using namespace archsim::translate::llvm;

static uint32_t InvalidTxln(void*)
{
	fprintf(stderr, "Attempted to execute invalid txln\n");
	abort();
}

LLVMTranslation::LLVMTranslation(translation_fn fnp, LLVMMemoryManager *mem_mgr) : fnp(fnp)
{
	if(mem_mgr) {
		code_size = mem_mgr->getAllocatedCodeSize();
		zones = mem_mgr->ReleasePages();
	}
}

LLVMTranslation::~LLVMTranslation()
{
	for(auto zone : zones) delete zone;
	fnp = InvalidTxln;
}

uint32_t LLVMTranslation::Execute(gensim::Processor& cpu)
{
	return fnp((void*)&cpu.state);
}

void LLVMTranslation::Install(jit_region_fn_table location)
{
	*location = *(jit_region_fn_table)&fnp;
}

uint32_t LLVMTranslation::GetCodeSize() const
{
	return code_size;
}