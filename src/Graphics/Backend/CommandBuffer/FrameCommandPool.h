// Purelly runtime class, allocates memory for command buffers derived from user created command buffer blueprints(scripting), than initializes and records all of them. 
#pragma once 
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include "CommandBufferBlueprint.h"
#include "FrameCommandBuffers.h"
#include "Utilities.h"
#include "Fence.h"

struct FrameCommandPool { 
	VkCommandPool vkHandle = VK_NULL_HANDLE;
    VkCommandBufferLevel level;  // Primary - can only be run by queue. Secondary - can only be called in another command buffer(using vkCmdExecureCommands(cmBuffer)).
	QueueFamilyEnum queueFamilyEnum; 
	
	FrameCommandBuffers commandBuffers;
	
    void create(VkCommandBufferLevel level_, QueueFamilyEnum queueFamilyEnum_);
    void resetCmdPool(Fence& rFrameFinishedFence_); // You need to rerecord buffers each draw, cause games are dynamic. An entity that uses distinct Graphics Pipeline might spawn, and invalidate previous recorded one. + Performance cost is negligable.  
	void recordCmdBuffers();
	void dealocateCmdBuffers();
    void destroy();
    VkCommandPool get() const; 

    void allocateCmdBuffersFromBlueprints();

    FrameCommandPool() = default;
};
