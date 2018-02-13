/*
 * File:   MemoryCacheEventHandler.h
 * Author: s0457958
 *
 * Created on 25 August 2014, 15:10
 */

#ifndef MEMORYCACHEEVENTHANDLER_H
#define	MEMORYCACHEEVENTHANDLER_H

#include "abi/memory/MemoryEventHandler.h"
#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "uarch/cache/MemoryCache.h"

#include <ostream>
#include <unordered_set>

namespace llvm
{
	class Value;
}

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace uarch
	{
		namespace cache
		{
			class MemoryCacheEventHandler : public archsim::abi::memory::MemoryEventHandler, public archsim::uarch::cache::MemoryCache
			{
			public:
				MemoryCacheEventHandler();

				bool HandleEvent(gensim::Processor& cpu, archsim::abi::memory::MemoryModel::MemoryEventType type, archsim::abi::memory::guest_addr_t addr, uint8_t size) override;
				void PrintGlobalCacheStatistics(std::ostream& stream);
				bool IsHit(CacheAccessType accessType, phys_addr_t phys_addr, virt_addr_t virt_addr, uint8_t size) override;

				archsim::abi::memory::MemoryEventHandlerTranslator& GetTranslator() override
				{
#if CONFIG_LLVM
					return translator;
#else
					assert(false);
#endif
				}

			private:
#if CONFIG_LLVM
				archsim::abi::memory::DefaultMemoryEventHandlerTranslator translator;
#endif

				void PrintStatisticsRecursive(std::ostream& stream, std::unordered_set<MemoryCache *>& seen, MemoryCache& cache);
			};
		}
	}
}

#endif	/* MEMORYCACHEEVENTHANDLER_H */
