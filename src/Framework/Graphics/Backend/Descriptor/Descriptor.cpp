#include "Descriptor.h"
#include "Api.h"
#include "DescriptorSetLayout.h"

namespace Graphics
{
	/* The descriptor layout lives in the DescriptorSetLayout this descriptor was derived from. Both ids are validated by Pool::operator[] under ENGINE_DEBUG. */
	DescriptorLayout& Descriptor::getDescriptorLayout()
	{
		return getDescriptorsManager().layouts[this->DSlayoutId].getDescriptorLayout(this->DescriptorLayoutId);
	}

	bool Descriptor::isDynamic()
	{
		return this->getDescriptorLayout().dynamic;
	}

	DescriptorTypeEnum Descriptor::getType()
	{
		return this->getDescriptorLayout().type;
	}

	/* A readOnly descriptor never changes after the lock and a GPU_LOCAL one is not copied per frame yet, so only mutable CPU_SHARED descriptors need a copy per frame in flight. */
	bool Descriptor::isPerFrame() const
	{
		return !this->readOnly && this->memoryVisability == CPU_SHARED;
	}
}
