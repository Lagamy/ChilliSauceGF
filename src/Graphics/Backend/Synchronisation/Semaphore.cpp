#include "Semaphore.h"
#include "Globals.h"

void Semaphore::create()
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
    vkHandle = VK_NULL_HANDLE;
}

Semaphore::~Semaphore()
{
    if (vkHandle != VK_NULL_HANDLE)
    {
        destroy();
    }
}

VkSemaphore& Semaphore::get() 
{
    return this->vkHandle;
}
