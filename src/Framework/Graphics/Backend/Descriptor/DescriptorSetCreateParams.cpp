#include "DescriptorSetCreateParams.h"
#include <stdexcept>

namespace Graphics
{
	void DescriptorSetCreateParams::validate() const
	{
		if(this->layoutId == UninitializedPoolId)
		{
			throw std::runtime_error("Descriptor Set Create Params: layoutId is required.");
		}
		if(this->allocatorType == ALLOCATOR_TYPE_UNSET)
		{
			throw std::runtime_error("Descriptor Set Create Params: allocatorType is required.");
		}
		if(this->memoryVisability == MEMORY_VISABILITY_UNSET)
		{
			throw std::runtime_error("Descriptor Set Create Params: memoryVisability is required.");
		}
		this->residency.validate(this->allocatorType);
	}
}
