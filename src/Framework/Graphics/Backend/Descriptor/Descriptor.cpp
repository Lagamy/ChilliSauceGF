#include "Descriptor.h"
#include "Api.h"

namespace Graphics
{
	/* The blueprint lives in the DescriptorSetLayout this descriptor was derived from. Both ids are validated by Pool::operator[] under ENGINE_DEBUG. */
	DescriptorBlueprint& Descriptor::getBlueprint()
	{
		return getDescriptorsManager().layouts[this->DSlayoutId].getDescriptorBlueprint(this->BlueprintId);
	}

	/* A readOnly descriptor never changes after the lock and a GPU_LOCAL one is not copied per frame yet, so only mutable CPU_SHARED descriptors need a copy per frame in flight. */
	bool Descriptor::isPerFrame() const
	{
		return !this->readOnly && this->memoryVisability == CPU_SHARED;
	}

	MemoryEntryId Descriptor::getEntryId(uint32_t frame_) const
	{
		return this->memoryEntryIds[this->isPerFrame() ? frame_ : 0];
	}
}
