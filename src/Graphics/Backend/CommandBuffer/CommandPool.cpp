#include "CommandPool.h"
#include "Globals.h"
#include <stdexcept>

void CommandPool::create(VkCommandBufferLevel level_, uint32_t queueFamilyIndex_)
{
    this->level = level_; 
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = queueFamilyIndex_; // In what queue command buffers from this pool - would be used. 
    VkResult result = vkCreateCommandPool(Demo::renderer.mainDevice.logicalDevice, &poolCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Command Pool!");
    }

	// Allocate command buffers from blueprints 
}

void CommandPool::destroy()
{
    vkDestroyCommandPool(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

void CommandPool::allocateIfActiveCommandBuffers()
{
    // Allocate CommandBuffers from the pool in GPU, and recieve handles for them.  
    if (this->rBlueprints.size() != 0)
    {
        std::vector<VkCommandBuffer> commandBufferHandles;
        this->commandBuffers.reserve(this->rBlueprints.size());
        commandBufferHandles.resize(this->rBlueprints.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = vkHandle; // Will only work on a queue of its Pool its from.  
        commandBufferAllocateInfo.level = this->level;
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBufferHandles.size());

        VkResult result = vkAllocateCommandBuffers(Demo::renderer.mainDevice.logicalDevice, &commandBufferAllocateInfo, commandBufferHandles.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Command Buffer/s!");
        }

        // Initialize our array with CommandBuffer objects
        for (size_t i = 0; i < commandBufferHandles.size(); i++)
        {
            this->commandBuffers.emplace_back(*this->rBlueprints[i], commandBufferHandles[i]);
        }
    }
}


VkCommandPool CommandPool::get() const {
    return this->vkHandle; 
}
