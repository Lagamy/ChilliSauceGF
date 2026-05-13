#pragma once 
#include<vulkan/vulkan.h>
#include <format>
#include <string>

struct MemoryBlock {
	VkDeviceMemory vkHandle;
	size_t size; 
	void create(VkMemoryRequirements memoryRequirements_, uint32_t memoryTypeIndex_, const char* name_);
	void uploadData(); // add to Command Buffer operation to copy data from CPU side cache to the GPU Host Visible buffer 
	void destroy();

	VkDeviceMemory get() const;


	uint32_t findMemoryTypeIndex(uint32_t allowedTypes_, VkMemoryPropertyFlags properties_);
};
