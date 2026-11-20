#pragma once
#include "PoolId.h"
#include "Utilities.h"
#include "Residency.h"
#include <cstdint>

namespace Graphics
{
	/*
		Everything needed to create a descriptor set, filled field by field like a Vulkan create info (start from `= {}`). Used by addDescriptorSet. validate() throws on a missing or contradicting field.

		layoutId, allocatorType, memoryVisability: required.
		residency: the on demand loading settings of the set, see ResidencyParams (every setting applies to all descriptors of the set, the load and unload itself is per descriptor). Only DYNAMIC_ALLOCATOR sets can unload, a static set has to set residency.absenceSecondsCount to UninitializedId. The disk functions get (setId, descriptorId), the ids Api.h functions take.
	*/
	struct DescriptorSetCreateParams
	{
		PoolId layoutId = UninitializedPoolId;
		AllocatorTypeEnum allocatorType = ALLOCATOR_TYPE_UNSET;
		MemoryVisabilityEnum memoryVisability = MEMORY_VISABILITY_UNSET;
		ResidencyParams residency;

		void validate() const; // throws std::runtime_error
	};
}
