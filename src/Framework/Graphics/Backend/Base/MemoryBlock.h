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
	VkDeviceSize size;
	VkDeviceSize freeSpace;
	std::vector<PoolId> bufferIds;
	uint32_t pageUpperBound;
	MemoryVisabilityEnum memVisability; 
	bool isStaging = false; 
	PoolId memBlockId;

	
	VkResult createInternal(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memPropertyIndex_);
	void createForStaticOrStaging(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memPropertyIndex_, MemoryVisabilityEnum memVisability_, bool isStaging_); 
	void create(size_t size_, std::span<VkMemoryRequirements> memReqsSpan_, uint32_t memPropertyIndex_, MemoryVisabilityEnum memVisability_, uint32_t pageUpperBound_, PoolId memBlockId_);

	void uploadData(); // add to Command Buffer operation to copy data from CPU side cache to the GPU Host Visible buffer
	void destroy();

	VkDeviceMemory get() const;
	uint32_t findMemoryTypeIndex(std::span<VkMemoryRequirements> memReqsSpan_, VkMemoryPropertyFlags properties_, bool isStatic_);

	MemoryBlock() = default;
	~MemoryBlock();
};
}
