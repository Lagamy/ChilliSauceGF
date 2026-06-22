// GPU - CPU sync(don't start CPU opperation if GPU data/state isn't ready)
#pragma once 
#include "vulkan/vulkan.h" 
#include <stdexcept>

namespace Graphics
{
struct Fence {
    VkFence vkHandle = VK_NULL_HANDLE;
    
    void destroy();
    const VkFence& get();
	Fence(VkFenceCreateFlags flags_);
	~Fence();
};
}
