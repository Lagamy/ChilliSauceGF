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

	switch(this->queueFamilyEnum)
	{
		case GRAPHICS: poolCreateInfo.queueFamilyIndex = getMainDevice().queueFamilyIndices.graphicsFamily; break; 
		case COMPUTE: poolCreateInfo.queueFamilyIndex = getMainDevice().queueFamilyIndices.computeFamily; break; 
		case TRANSFER: poolCreateInfo.queueFamilyIndex = getMainDevice().queueFamilyIndices.transferFamily; break; 
	}

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
	std::vector<Pass>& rPasses = getPassesManager().passesPerCmdType[FRAME].passesPerQueue[this->queueFamilyEnum];  
    for(auto& rPass : rPasses)
    {
        this->commandBuffers.commandsToRecord.resize(this->commandBuffers.commandsToRecord.size() + rPass.tasks.size());
        size_t baseSize = this->commandBuffers.buffers.size(); 
        for(size_t i = 0; i < rPass.tasks.size(); i++)
        {
            this->commandBuffers.commandsToRecord[baseSize + i] = rPass.tasks[i].cmdBufferFunc; 
            rPass.tasks[i].cmdId = baseSize + i; 
        }
        
    }
    
    this->commandBuffers.buffers.resize(this->commandBuffers.commandsToRecord.size()); 
    this->commandBuffers.buffers.resize(this->commandBuffers.recorded.size()); 
    
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



void OneShotCommandPool::resetCmdBuffer(uint32_t id_, Fence& rFinishSignalFence_)
{
	vkWaitForFences(getMainDevice().logicalDevice, 1, &rFinishSignalFence_.get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetCommandBuffer(this->commandBuffers.buffers[id_], 0); 
	this->commandBuffers.recorded[id_] = false;
}

void OneShotCommandPool::recordCmdBuffer(uint32_t id_)
{
	this->commandBuffers.commandsToRecord[id_](this->commandBuffers.buffers[id_]);
	this->commandBuffers.recorded[id_] = true;
}


VkCommandPool& OneShotCommandPool::get() {
    return this->vkHandle; 
}
}
