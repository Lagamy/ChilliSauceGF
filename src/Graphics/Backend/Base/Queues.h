#pragma once 
#include <vulkan/vulkan.h>

namespace Graphics
{
struct Queues {
	// Handles that will contain references from GPU for that queues. Channels to talk to the GPU for graphics/transfer/present commands
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue transferQueue = VK_NULL_HANDLE;
	VkQueue computeQueue = VK_NULL_HANDLE; 
	VkQueue presentQueue = VK_NULL_HANDLE;
};
}

