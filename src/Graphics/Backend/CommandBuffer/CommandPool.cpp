#include "CommandPool.h"
#include "CommandBufferBlueprint.h"
#include "Globals.h"
#include "Utilities.h"
#include <stdexcept>

void CommandPool::create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_)
{
	this->queueFamilyEnum = queueFamilyEnum_; 
    this->level = level_; 
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	switch(this->queueFamilyEnum)
	{
		case GRAPHICS: poolCreateInfo.queueFamilyIndex = Demo::renderer.mainDevice.queueFamilyIndicies.graphicsFamily; break; 
		case COMPUTE: poolCreateInfo.queueFamilyIndex = Demo::renderer.mainDevice.queueFamilyIndicies.computeFamily; break; 
		case TRANSFER: poolCreateInfo.queueFamilyIndex = Demo::renderer.mainDevice.queueFamilyIndicies.transferFamily; break; 
	}

    VkResult result = vkCreateCommandPool(Demo::renderer.mainDevice.logicalDevice, &poolCreateInfo, nullptr, &this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Command Pool!");
    }

	// Allocate command buffers from blueprints 
	this->allocateIfActiveCommandBuffers();
}

void CommandPool::destroy()
{
    vkDestroyCommandPool(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

void CommandPool::allocateIfActiveCommandBuffers()
{
    // Allocate CommandBuffers from the pool in GPU, and recieve handles for them. 
	std::vector<CommandBufferBlueprint>& rBlueprints = 
		this->queueFamilyEnum == GRAPHICS ? Demo::renderer.renderFlow.graphicsCmdBufferBlueprints : 
		this->queueFamilyEnum == COMPUTE ? Demo::renderer.renderFlow.computeCmdBufferBlueprints : 
		Demo::renderer.renderFlow.transferCmdBufferBlueprints; 
		
    if (rBlueprints.size() != 0)
    {
        std::vector<VkCommandBuffer> commandBufferHandles;
        this->commandBuffers.reserve(rBlueprints.size());
        commandBufferHandles.resize(rBlueprints.size());

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
            this->commandBuffers.emplace_back(rBlueprints[i], commandBufferHandles[i]);
        }
    }
}


VkCommandPool CommandPool::get() const {
    return this->vkHandle; 
}
