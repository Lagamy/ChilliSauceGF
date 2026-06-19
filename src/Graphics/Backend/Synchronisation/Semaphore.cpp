#include "Semaphore.h"
#include "Globals.h"

namespace Graphics
{
Semaphore::Semaphore()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkResult result = vkCreateSemaphore(Demo::renderer.mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Semaphore!");
    }
}

void Semaphore::destroy()
{
    vkDestroySemaphore(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

Semaphore::~Semaphore()
{
    if (this->vkHandle != VK_NULL_HANDLE)
    {
        this->destroy();
    }
}

VkSemaphore& Semaphore::get() 
{
    return this->vkHandle;
}
}
