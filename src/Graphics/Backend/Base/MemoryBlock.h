#pragma once 
#include <cstddef>
#include<vulkan/vulkan.h>
#include <format>
#include <string>

namespace Graphics
{
enum StorageUnitEnum : uint8_t 
{ 
	BYTE = 0,
	KB = 1,
	MB = 2,
	GB = 3
};

struct MemoryBlock {
	VkDeviceMemory vkHandle;
	void create(size_t size_, StorageUnitEnum unit_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_, const char* name_);

	VkDeviceSize toBytes(size_t size_, StorageUnitEnum unit_);
	void uploadData(); // add to Command Buffer operation to copy data from CPU side cache to the GPU Host Visible buffer 
	void destroy();

	VkDeviceMemory get() const;


	uint32_t findMemoryTypeIndex(std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_, const char* name_);
};
}
