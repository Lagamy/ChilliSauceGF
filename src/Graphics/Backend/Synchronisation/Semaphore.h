// GPU - GPU sync(don't start GPU opperation, if another required GPU operation haven't finished yet)
#pragma once 
#include <vulkan/vulkan.h>
#include <stdexcept>

namespace Graphics
{
struct Semaphore {
    VkSemaphore vkHandle = VK_NULL_HANDLE;

    void destroy();
    VkSemaphore& get();
	Semaphore();
    ~Semaphore();
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(Semaphore&& other_) noexcept
    {
        this->vkHandle = other_.vkHandle;
        other_.vkHandle = VK_NULL_HANDLE;
    }
    
    Semaphore& operator=(Semaphore&& other_) noexcept
    {
        if (this != &other_)
        {
            destroy();
            vkHandle = other_.vkHandle;
            other_.vkHandle = VK_NULL_HANDLE;
        }
        return *this;
    };
};
}
