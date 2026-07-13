#include "Semaphore.h"
#include "Api.h"

namespace Graphics
{
void Semaphore::create()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkResult result = vkCreateSemaphore(getMainDevice().logicalDevice, &semaphoreCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Semaphore!");
    }
}

void Semaphore::destroy()
{
    if (this->vkHandle != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(getMainDevice().logicalDevice, this->vkHandle, nullptr);
        this->vkHandle = VK_NULL_HANDLE;
    }
}

VkSemaphore& Semaphore::get() 
{
    return this->vkHandle;
}
}
