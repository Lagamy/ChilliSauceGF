#include "Fence.h"
#include "Api.h"

namespace Graphics
{
void Fence::create(bool createSignaled_)
{
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if(createSignaled_) fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

    VkResult result = vkCreateFence(getMainDevice().logicalDevice, &fenceCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Fence!");
    }
}


void Fence::destroy()
{
    if (this->vkHandle != VK_NULL_HANDLE)
    {
        vkDestroyFence(getMainDevice().logicalDevice, this->vkHandle, nullptr);
	    this->vkHandle = VK_NULL_HANDLE;
    }
}


VkFence& Fence::get() 
{
    return this->vkHandle;
}
}
