#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <format>


namespace Graphics
{
	struct Buffer {
		VkBuffer vkHandle = VK_NULL_HANDLE;
		/* Neutral requirements for a buffer that was never created (size 0): no space, no alignment constraint, any memory type. */
		VkMemoryRequirements memoryReqs = { 0, 1, ~0u };
		// PoolId memoryBlockId; 
		uint64_t size; 

		void create(VkDeviceSize size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_);
		void destroy(); // temporary. Will move into destructor later, once I figure out - how i want my resource managment to be structured
		VkBuffer get() const;
		Buffer() = default; 
		Buffer(VkDeviceSize size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_);
	};
}
