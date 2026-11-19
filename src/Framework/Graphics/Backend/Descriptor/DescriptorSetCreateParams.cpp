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
		if(this->absenceSecondsCount == 0)
		{
			throw std::runtime_error("Descriptor Set Create Params: absenceSecondsCount can't be 0, the descriptor would unload and load again every frame. Use 1 or more, or UninitializedId to never unload.");
		}
		if(this->allocatorType == STATIC_ALLOCATOR &&this->absenceSecondsCount != UninitializedId)
		{
			throw std::runtime_error("Descriptor Set Create Params: A STATIC_ALLOCATOR set can't unload (static entries can't be removed), set absenceSecondsCount to UninitializedId.");
		}
	}
}
