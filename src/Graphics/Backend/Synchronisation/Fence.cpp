#include "Fence.h"
#include "Api.h"

namespace Graphics
{
Fence::Fence(VkFenceCreateFlags flags_)
{
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = flags_; 

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
