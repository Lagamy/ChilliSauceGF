// GPU - GPU sync(don't start GPU opperation, if another required GPU operation haven't finished yet)
#pragma once 
#include <vulkan/vulkan.h>
#include <stdexcept>

namespace Graphics
{
struct Semaphore {
    VkSemaphore vkHandle = VK_NULL_HANDLE;

    void create(); 
    void destroy();
    VkSemaphore& get();

};
}
