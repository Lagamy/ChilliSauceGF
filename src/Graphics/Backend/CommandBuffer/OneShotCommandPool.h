// Purelly runtime class, allocates memory for command buffers derived from user created command buffer blueprints(scripting), than initializes and records all of them. 
#pragma once 
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include "OneShotCommandBuffers.h"
#include "Utilities.h"
#include "Fence.h"

namespace Graphics
{
struct OneShotCommandPool { 
	VkCommandPool vkHandle = VK_NULL_HANDLE;
    VkCommandBufferLevel level;  // Primary - can only be run by queue. Secondary - can only be called in another command buffer(using vkCmdExecureCommands(cmBuffer)).
	QueueFamilyEnum queueFamilyEnum; 

    OneShotCommandBuffers commandBuffers; 

    void create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_);
    void resetCmdBuffer(uint32_t id_, Fence& rFinishSignalFence_); 
	void recordCmdBuffer(uint32_t id_);
	void submitCmdBuffer(uint32_t id_);
	void dealocateCMDBuffers();
    void destroy();
    VkCommandPool& get(); 

    void allocateCmdBuffersFromBlueprints();

    OneShotCommandPool() = default;
};
}
