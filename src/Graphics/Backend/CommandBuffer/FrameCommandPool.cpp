#include "FrameCommandPool.h"
#include "Api.h"

namespace Graphics
{
void FrameCommandPool::create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_)
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
	this->allocateCmdBuffersFromBlueprints();
}

void FrameCommandPool::destroy()
{
    vkDestroyCommandPool(getMainDevice().logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

void FrameCommandPool::allocateCmdBuffersFromBlueprints()
{
    // Allocate CommandBuffers from the pool in GPU, and recieve handles for them. 
	std::vector<CommandBufferBlueprint>& rBlueprints = 
		this->queueFamilyEnum == GRAPHICS ? getCmdBufferBlueprints(FRAME).graphics : 
		this->queueFamilyEnum == COMPUTE ? getCmdBufferBlueprints(FRAME).compute : 
		getCmdBufferBlueprints(FRAME).transfer; 
		
    if (rBlueprints.size() != 0)
    {
        std::vector<VkCommandBuffer> commandBufferHandles;
 		this->commandBuffers.buffers.resize(rBlueprints.size());
        this->commandBuffers.commandsToRecord.resize(rBlueprints.size());
        commandBufferHandles.resize(rBlueprints.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = vkHandle; // Will only work on a queue of its Pool its from.  
        commandBufferAllocateInfo.level = this->level;
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBufferHandles.size());

        VkResult result = vkAllocateCommandBuffers(getMainDevice().logicalDevice, &commandBufferAllocateInfo, commandBufferHandles.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Command Buffer/s!");
        }

        // Initialize our array with CommandBuffer objects
        for (size_t i = 0; i < commandBufferHandles.size(); i++)
        {
            this->commandBuffers.buffers[i] = commandBufferHandles[i];
			this->commandBuffers.commandsToRecord[i] = rBlueprints[i].commandsToRecord; 
        }
    }
}

void FrameCommandPool::resetCmdPool()
{

	vkResetCommandPool(getMainDevice().logicalDevice, this->vkHandle, 0); 
}

void FrameCommandPool::recordCmdBuffers()
{
	for(uint32_t i = 0; i < this->commandBuffers.buffers.size(); i++)
	{
		this->commandBuffers.commandsToRecord[i](this->commandBuffers.buffers[i]);
	}
}

const VkCommandPool& FrameCommandPool::get() {
    return this->vkHandle; 
}
}
