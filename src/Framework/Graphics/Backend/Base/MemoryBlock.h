#pragma once 
#include "UploadId.h"
#include "Utilities.h"
#include <cstddef>
#include <vulkan/vulkan.h>
#include <format>
#include <string>

namespace Graphics
{
struct MemoryBlock {
	VkDeviceMemory vkHandle;
	bool isStatic = false; 
	VkDeviceSize size; 
	VkDeviceSize freeSpace; 
	std::vector<PoolId> bufferIds; 
	const char* uploadName; 

	void createForStatic(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_);
	void create(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_, const char* uploadName_);

	void uploadData(); // add to Command Buffer operation to copy data from CPU side cache to the GPU Host Visible buffer 
	void destroy();

	VkDeviceMemory get() const;
	uint32_t findMemoryTypeIndex(std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_);

	MemoryBlock() = default; 
	MemoryBlock(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memoryTypeIndex_, const char* uploadName_);
	~MemoryBlock(); 
};
}
