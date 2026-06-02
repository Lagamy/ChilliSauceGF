#include "Fence.h"
#include "Globals.h"
#include <vulkan/vulkan_core.h>

void Fence::create()
{
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Makes Fence signaled once created

    VkResult result = vkCreateFence(Demo::renderer.mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Fence!");
    }
}



void Fence::destroy()
{
    vkDestroyFence(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
}


const VkFence& Fence::get() 
{
    return this->vkHandle;
}
