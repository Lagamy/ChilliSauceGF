#pragma once
#include <vulkan/vulkan.h>

namespace Graphics
{
struct PendingCopy
{
	VkBuffer srcBuffer = VK_NULL_HANDLE;
	VkBuffer dstBuffer = VK_NULL_HANDLE;
	VkDeviceSize srcOffset = 0;
	VkDeviceSize dstOffset = 0;
	VkDeviceSize size = 0;
	bool isReadback = false; // The copy reads a GPU Local entry buffer into a readback range, instead of writing it.
};
}
