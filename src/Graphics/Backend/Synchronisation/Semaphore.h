// GPU - GPU sync(don't start GPU opperation, if another required GPU operation haven't finished yet)
#pragma once 
#include <vulkan/vulkan.h>
#include <stdexcept>

struct Semaphore {
private:
    VkSemaphore vkHandle = VK_NULL_HANDLE;
public: 
    void create();
    void destroy();
    ~Semaphore();
    VkSemaphore get() const;
};
