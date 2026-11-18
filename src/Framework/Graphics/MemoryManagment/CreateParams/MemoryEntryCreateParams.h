#pragma once
#include "MemoryAccessParams.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
	/*
		Everything needed to create a memory entry, filled field by field like a Vulkan create info (start from `= {}`).
		Used by MemoryManager::addEntry / allocateEntry / reserveEntry and the Api.h forwarders. validate() throws on a missing or contradicting field.

		name, allocatorType, memoryVisability, bufferType, size, access.accessMode: required (the enums start at their ..._UNSET value, validate throws if it is still there).
		data: the bytes to upload. Required by the add functions, has to stay nullptr for allocate / reserve (nothing is uploaded, the entry is written later).
		access: see MemoryAccessParams.
	*/
	struct MemoryEntryCreateParams
	{
		const char* name = nullptr;
		AllocatorTypeEnum allocatorType = ALLOCATOR_TYPE_UNSET;
		MemoryVisabilityEnum memoryVisability = MEMORY_VISABILITY_UNSET;
		BufferTypeEnum bufferType = BUFFER_TYPE_UNSET;
		VkDeviceSize size = 0;
		const void* data = nullptr;
		MemoryAccessParams access;

		void validate() const; // throws std::runtime_error
	};
}
