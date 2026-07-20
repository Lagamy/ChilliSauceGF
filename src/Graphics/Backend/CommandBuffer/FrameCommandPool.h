// Purelly runtime class, allocates memory for command buffers derived from user created command buffer blueprints(scripting), than initializes and records all of them. 
#pragma once 
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include "CmdBuffersInPasses.h"
#include "Utilities.h"
#include "Fence.h"

namespace Graphics
{
struct FrameCommandPool { 
	VkCommandPool vkHandle = VK_NULL_HANDLE;
    VkCommandBufferLevel level;  // Primary - can only be run by queue. Secondary - can only be called in another command buffer(using vkCmdExecureCommands(cmBuffer)).
	QueueFamilyEnum queueFamilyEnum; 
	
	CmdBuffersInPasses commandBuffers;
	
    void create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_);
	void rerecordEnabledCmdBuffers();
	void dealocateCmdBuffers();
    void destroy();
    const VkCommandPool& get(); 

    void allocateCmdBuffersFromPasses();

    FrameCommandPool() = default;
};
}
