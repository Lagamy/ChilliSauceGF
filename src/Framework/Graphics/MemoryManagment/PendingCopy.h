#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Graphics
{
/* What the copy does, MemoryManager records a memory dependency wherever the kind switches (see uploadQueuedCopiesCMDs). */
enum class PendingCopyKindEnum : uint8_t
{
	UPDATE, // staging range -> GPU Local entry buffer
	READBACK, // GPU Local entry buffer -> readback range
	ENTRY_COPY // GPU Local entry buffer -> another GPU Local entry buffer (MemoryManager::copyEntry)
};

struct PendingCopy
{
	VkBuffer srcBuffer = VK_NULL_HANDLE;
	VkBuffer dstBuffer = VK_NULL_HANDLE;
	VkDeviceSize srcOffset = 0;
	VkDeviceSize dstOffset = 0;
	VkDeviceSize size = 0;
	PendingCopyKindEnum kind = PendingCopyKindEnum::UPDATE;
};
}
