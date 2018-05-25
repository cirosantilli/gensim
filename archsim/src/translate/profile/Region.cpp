#include "translate/Translation.h"
#include "translate/TranslationManager.h"
#include "translate/profile/Region.h"
#include "translate/profile/Block.h"
#include "util/LogContext.h"

#include <mutex>

UseLogContext(LogTranslate);
UseLogContext(LogLifetime);

using namespace archsim::translate::profile;

Region::Region(TranslationManager& mgr, phys_addr_t phys_base_addr)
	:	mgr(mgr),
	  phys_base_addr(phys_base_addr),
	  current_generation(0),
	  max_generation(0),
	  status(NotInTranslation),
	  invalid(false),
	  txln(NULL)
{

}

Region::~Region()
{
	LC_DEBUG2(LogLifetime) << "Deleting Region Object: " << *this;

	if(txln) {
		mgr.RegisterTranslationForGC(*txln);
	}
}

void Region::dump()
{
	std::cerr << *this;
}

void Region::dump_dot()
{
	std::cerr << "graph {" << std::endl;;
	
	for(auto i : blocks) {
		std::cerr << "block_" << std::hex << i.first << ";" << std::endl;
	}
	
	for(auto i : blocks) {
		for(auto j : i.second->GetSuccessors()) {
			std::cerr << "block_" << std::hex << i.first << " -> block_" << std::hex << j->GetOffset() << ";" << std::endl;
		}
	}
	
	std::cerr << "}" << std::endl;;
}


Block& Region::GetBlock(Address virt_addr, uint8_t isa_mode)
{
	addr_t offset = virt_addr.GetPageOffset();

	Block *&block = blocks[offset];
	if (UNLIKELY(!block)) {
		block = block_zone.Construct(*this, offset, isa_mode);
	}

	assert(block->GetISAMode() == isa_mode);
	return *block;
}

void Region::EraseBlock(virt_addr_t virt_addr)
{
	addr_t offset = RegionArch::PageOffsetOf(virt_addr);
	blocks.erase(offset);
}

bool Region::HasTranslations() const
{
	return (GetStatus() == InTranslation) || txln;
}

void Region::TraceBlock(archsim::core::thread::ThreadInstance *thread, Address virt_addr)
{
	if (status == InTranslation)
		return;

	virtual_images.insert(virt_addr.GetPageBase());
	block_interp_count[virt_addr.GetPageOffset()]++;
	total_interp_count++;

	mgr.TraceBlock(thread, GetBlock(virt_addr, thread->GetModeID()));
}

void Region::Invalidate()
{
	invalid = true;
}

size_t Region::GetApproximateMemoryUsage() const
{
	size_t size = sizeof(*this);

	for(auto i : virtual_images) size += sizeof(i);
	for(auto i : block_interp_count) size += sizeof(i);
	for(auto i : blocks) size += sizeof(i);
	size += block_zone.GetAllocatedSize();

	return size;
}

namespace archsim
{
	namespace translate
	{
		namespace profile
		{

			std::ostream& operator<< (std::ostream& out, Region& rgn)
			{
				out << "[Region " << std::hex << rgn.phys_base_addr << "(" << &rgn << "), generation=" << std::dec << rgn.current_generation << "/" << rgn.max_generation;

				if (rgn.invalid)
					out << " INVALID";

//	out << ", Heat = " << rgn.TotalBlockHeat() << "/" << rgn.TotalRegionHeat();

				out << "]";
				return out;
			}

		}
	}
}
