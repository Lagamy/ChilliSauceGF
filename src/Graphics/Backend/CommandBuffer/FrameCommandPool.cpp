#include "FrameCommandPool.h"
#include "Api.h"
#include "Utilities.h"

namespace Graphics
{
void FrameCommandPool::create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_)
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

void FrameCommandPool::destroy()
{
    vkDestroyCommandPool(getMainDevice().logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

void FrameCommandPool::allocateCmdBuffersFromPasses()
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

void FrameCommandPool::resetCmdPool()
{

	vkResetCommandPool(getMainDevice().logicalDevice, this->vkHandle, 0); 
}

void FrameCommandPool::recordCmdBuffers()
{
	for(const auto& enabledCmdBufId : this->commandBuffers.enabled)
	{
		this->commandBuffers.commandsToRecord[enabledCmdBufId](this->commandBuffers.buffers[enabledCmdBufId]);
	}
}

const VkCommandPool& FrameCommandPool::get() {
    return this->vkHandle; 
}
}
