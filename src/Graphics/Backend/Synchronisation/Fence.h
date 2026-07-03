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

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Fence&& other) noexcept
    {
        vkHandle = other.vkHandle;
        other.vkHandle = VK_NULL_HANDLE;
    }
    
    Fence& operator=(Fence&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            vkHandle = other.vkHandle;
            other.vkHandle = VK_NULL_HANDLE;
        }
        return *this;
    };
};
}
