#include "OneShotCommandPool.h"
#include "Api.h"
#include "Utilities.h"
#include <limits>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void OneShotCommandPool::create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_)
{
	this->queueFamilyEnum = queueFamilyEnum_; 
    this->level = level_; 
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = getMainDevice().queueFamilyIndices.indices[queueFamilyEnum_]; 

    VkResult result = vkCreateCommandPool(getMainDevice().logicalDevice, &poolCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Command Pool!");
    }

	// Allocate command buffers from blueprints 
	this->allocateCmdBuffersFromPasses();
}

void OneShotCommandPool::destroy()
{
    vkDestroyCommandPool(getMainDevice().logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

void OneShotCommandPool::allocateCmdBuffersFromPasses()
{
    // Allocate CommandBuffers from the pool in GPU, and recieve handles for them. 
    this->commandBuffers.buffers.resize(this->commandBuffers.commandsToRecord.size()); 
    
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vkHandle; // Will only work on a queue of its Pool its from.  
    commandBufferAllocateInfo.level = this->level;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.buffers.size());

    VkResult result = vkAllocateCommandBuffers(getMainDevice().logicalDevice, &commandBufferAllocateInfo, this->commandBuffers.buffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Command Buffer/s!");
    }
}

void OneShotCommandPool::rerecordEnabledCmdBuffers()
{
	for(uint32_t i = 0; i < this->commandBuffers.enabled.size(); i++)
	{

	    vkResetCommandBuffer(this->commandBuffers.buffers[this->commandBuffers.enabled[i]], 0); 
        uint32_t& enabledCmdBufId = this->commandBuffers.enabled[i]; 
		this->commandBuffers.commandsToRecord[enabledCmdBufId](this->commandBuffers.buffers[enabledCmdBufId]);
        this->commandBuffers.enabled.erase(this->commandBuffers.enabled.begin() + i); 
    }
}


VkCommandPool& OneShotCommandPool::get() {
    return this->vkHandle; 
}
}
