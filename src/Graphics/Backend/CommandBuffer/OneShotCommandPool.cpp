#include "OneShotCommandPool.h"
#include "Globals.h"
#include <limits>
#include <vulkan/vulkan_core.h>

void OneShotCommandPool::create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_)
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
	this->allocateCmdBuffersFromBlueprints();
}

void OneShotCommandPool::destroy()
{
    vkDestroyCommandPool(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
    this->vkHandle = VK_NULL_HANDLE;
}

void OneShotCommandPool::allocateCmdBuffersFromBlueprints()
{
    // Allocate CommandBuffers from the pool in GPU, and recieve handles for them. 
	std::vector<CommandBufferBlueprint>& rBlueprints = 
		this->queueFamilyEnum == GRAPHICS ? Demo::renderer.renderFlow.oneShotCmdBufferBlueprints.graphics: 
		this->queueFamilyEnum == COMPUTE ? Demo::renderer.renderFlow.oneShotCmdBufferBlueprints.compute: 
		Demo::renderer.renderFlow.oneShotCmdBufferBlueprints.transfer; 
		
    if (rBlueprints.size() != 0)
    {
        std::vector<VkCommandBuffer> commandBufferHandles;
        this->commandBuffers.buffers.reserve(rBlueprints.size());
        this->commandBuffers.commandsToRecord.reserve(rBlueprints.size());

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
            this->commandBuffers.buffers.emplace_back(commandBufferHandles[i]);
			this->commandBuffers.commandsToRecord.emplace_back(rBlueprints[i].commandsToRecord);
        }
    }
}

void OneShotCommandPool::resetCmdBuffer(uint32_t id_)
{
	vkWaitForFences(Demo::renderer.mainDevice.logicalDevice, 1, &this->commandBuffers.finished[id_].get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetCommandBuffer(this->commandBuffers.buffers[id_], 0); 
	this->commandBuffers.recorded[id_] = false;
}

void OneShotCommandPool::recordCmdBuffer(uint32_t id_)
{
	this->commandBuffers.commandsToRecord[id_](this->commandBuffers.buffers[id_]);
	this->commandBuffers.recorded[id_] = true;
}


VkCommandPool OneShotCommandPool::get() const {
    return this->vkHandle; 
}
