#pragma once 
#include "Utilities.h"
#include <cstddef>
#include <vulkan/vulkan.h>
#include <format>
#include <string>

namespace Graphics
{
struct MemoryBlock {
	VkDeviceMemory vkHandle;

	VkDeviceSize size; 
	VkDeviceSize occupiedSpace; 
	VkDeviceSize freeSpace; 
	std::vector<PoolId> bufferIds; 

	void create(size_t size_, StorageUnitEnum unit_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_, const char* context_);

	VkDeviceSize toBytes(size_t size_, StorageUnitEnum unit_);
	void uploadData(); // add to Command Buffer operation to copy data from CPU side cache to the GPU Host Visible buffer 
	void destroy();

	VkDeviceMemory get() const;
	uint32_t findMemoryTypeIndex(std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_, const char* context_);

	MemoryBlock() = default; 
	MemoryBlock(size_t size_, StorageUnitEnum unit_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_, const char* context_);
	~MemoryBlock(); 
};
}
