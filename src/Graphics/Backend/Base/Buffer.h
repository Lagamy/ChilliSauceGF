#pragma once 
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <format>


namespace Graphics
{
	struct Buffer {
		VkBuffer vkHandle = VK_NULL_HANDLE;
		VkMemoryRequirements memoryReqs; 
		
		void create(VkDeviceSize size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_);
		void destroy(); // temporary. Will move into destructor later, once I figure out - how i want my resource managment to be structured
		VkBuffer get() const;

		~Buffer();
	};
}
