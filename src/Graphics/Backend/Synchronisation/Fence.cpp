#include "Fence.h"
#include "Api.h"

namespace Graphics
{
Fence::Fence()
{
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Makes Fence signaled once created

    VkResult result = vkCreateFence(getMainDevice().logicalDevice, &fenceCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Fence!");
    }
}


void Fence::destroy()
{
    vkDestroyFence(getMainDevice().logicalDevice, this->vkHandle, nullptr);
	this->vkHandle = VK_NULL_HANDLE;
}


const VkFence& Fence::get() 
{
    return this->vkHandle;
}

Fence::~Fence()
{
	if (this->vkHandle != VK_NULL_HANDLE)
    {
        this->destroy();
    }	
}
}
