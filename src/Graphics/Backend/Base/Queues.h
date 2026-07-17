#pragma once 
#include <vulkan/vulkan.h>
#include <array>

namespace Graphics
{
struct Queues {
	// Handles that will contain references from GPU for that queues. Channels to talk to the GPU for graphics/transfer/present commands
	std::array<VkQueue, 4> queues = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};  
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue transferQueue = VK_NULL_HANDLE;
	VkQueue computeQueue = VK_NULL_HANDLE; 
	VkQueue presentQueue = VK_NULL_HANDLE;
};
}

