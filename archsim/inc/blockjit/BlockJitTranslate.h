/*
 * BlockJitTranslate.h
 *
 *  Created on: 20 Aug 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCKJITTRANSLATE_H_
#define INC_BLOCKJIT_BLOCKJITTRANSLATE_H_

#include "abi/Address.h"
#include "translate/profile/Region.h"
#include "blockjit/translation-context.h"
#include "gensim/ProcessorFeatures.h"

#include "arch/arm/ARMDecodeContext.h"

#include <unordered_set>
#include <set>

namespace captive
{
	namespace shared
	{
		class IROperand;
	}
}

namespace archsim
{
	namespace blockjit
	{
		class BlockTranslation;
	}
}

namespace wulib
{
	class MemAllocator;
}

namespace gensim
{
	class BaseDecode;
	class BlockJitProcessor;
	class BaseJumpInfo;

	namespace blockjit
	{
		class BaseBlockJITTranslate
		{
		public:
			BaseBlockJITTranslate();
			virtual ~BaseBlockJITTranslate();

			bool translate_block(gensim::BlockJitProcessor *processor, archsim::VirtualAddress block_address, archsim::blockjit::BlockTranslation &out_txln, wulib::MemAllocator &allocator);

			void setSupportChaining(bool enable);
			void setSupportProfiling(bool enable);
			void setTranslationMgr(archsim::translate::TranslationManager *txln_mgr);

			void InitialiseFeatures(const gensim::BlockJitProcessor *cpu);
			void SetFeatureLevel(uint32_t feature, uint32_t level, captive::arch::jit::TranslationContext& ctx);
			uint32_t GetFeatureLevel(uint32_t feature);
			void InvalidateFeatures();
			archsim::ProcessorFeatureSet GetProcessorFeatures() const;

			void InitialiseIsaMode(const gensim::BlockJitProcessor *cpu);
			void SetIsaMode(const captive::shared::IROperand&, captive::arch::jit::TranslationContext& ctx);
			uint32_t GetIsaMode();
			void InvalidateIsaMode();

		protected:
			virtual bool translate_instruction(const BaseDecode* decode_obj, captive::arch::jit::TranslationContext& ctx, bool trace) = 0;

		private:
			typedef archsim::VirtualAddress VirtualAddress;

			std::map<uint32_t, uint32_t> _feature_levels;
			std::map<uint32_t, uint32_t> _initial_feature_levels;
			std::set<uint32_t> _read_feature_levels;
			bool _features_valid;

			uint32_t _isa_mode;
			bool _isa_mode_valid;

			//ARM HAX
			gensim::DecodeContext *_decode_ctx;
			gensim::DecodeTranslateContext *decode_txlt_ctx;

			bool _supportChaining;
			gensim::BaseDecode *_decode;
			gensim::BaseJumpInfo *_jumpinfo;

			archsim::translate::TranslationManager *_txln_mgr;
			bool _supportProfiling;

			bool _should_be_dumped;

			bool build_block(gensim::BlockJitProcessor *cpu, VirtualAddress block_address, captive::arch::jit::TranslationContext &ctx);
			bool compile_block(gensim::BlockJitProcessor *cpu, VirtualAddress block_address, captive::arch::jit::TranslationContext &ctx, captive::shared::block_txln_fn &fn, wulib::MemAllocator &allocator);

			bool emit_block(gensim::BlockJitProcessor *cpu, VirtualAddress block_address, captive::arch::jit::TranslationContext &ctx, std::unordered_set<VirtualAddress> &block_heads);
			bool emit_instruction(gensim::BlockJitProcessor *cpu, VirtualAddress pc, gensim::BaseDecode *insn, captive::arch::jit::TranslationContext &ctx);
			bool emit_chain(gensim::BlockJitProcessor *cpu, VirtualAddress block_address, gensim::BaseDecode *insn, captive::arch::jit::TranslationContext &ctx);

			bool can_merge_jump(gensim::BlockJitProcessor *cpu, gensim::BaseDecode *decode, VirtualAddress pc);
			VirtualAddress get_jump_target(gensim::BlockJitProcessor *cpu, gensim::BaseDecode *decode, VirtualAddress pc);
		};
	}
}

#endif /* INC_BLOCKJIT_BLOCKJITTRANSLATE_H_ */