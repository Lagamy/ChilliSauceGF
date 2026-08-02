// Purelly runtime class, allocates memory for command buffers derived from user created command buffer blueprints(scripting), than initializes and records all of them. 
#pragma once 
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include "CmdBuffers.h"
#include "Utilities.h"

namespace Graphics
{
struct OneShotCommandPool { 
	VkCommandPool vkHandle = VK_NULL_HANDLE;
    VkCommandBufferLevel level;  // Primary - can only be run by queue. Secondary - can only be called in another command buffer(using vkCmdExecureCommands(cmBuffer)).
	QueueFamilyEnum queueFamilyEnum; 

    CmdBuffers commandBuffers; 

    void create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_);
	void rerecordEnabledCmdBuffers();
	void submitCmdBuffer(uint32_t id_);
	void dealocateCMDBuffers();
    void destroy();
    VkCommandPool& get(); 

    void allocateCmdBuffersFromPasses();

    OneShotCommandPool() = default;
};
}
