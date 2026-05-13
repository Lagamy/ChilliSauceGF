// Record all commands here(Start Renderpass -> Bind a Graphics Pipeline -> Bind Vertex/Index data -> Bind Descriptor Sets and Push Constants -> Draw) 
// To than send it to Graphics queue for GPU to execute

//Per frame - in - flight:
//  • 1 primary command buffer(recorded on main thread)
//  └─ vkCmdBeginRenderPass(..., framebuffer = swapchainImage[currentFrame])
//  └─ vkCmdBindPipeline(...)
//  └─ vkCmdBindVertexBuffers(...)
//  └─ vkCmdBindDescriptorSets(...)
//  └─ vkCmdDraw(...) or vkCmdExecuteCommands(secondaryCmdBuffers[])
//  └─ vkCmdEndRenderPass()
// Created from CommandBufferLatour 
#pragma once 
#include <vulkan/vulkan.h>
#include <stdexcept>
#include "CommandBufferBlueprint.h"

struct CommandBuffer {
    VkCommandBuffer vkHandle = VK_NULL_HANDLE; // Gets created externally by Command Pool 

	Cmd commandsToRecord; 
	VkCommandBufferUsageFlags cmdBufferUsageFlags; // Has options like: "One Time Submit", "Render Pass Continue(Secondary CmdBuffer)" and "Simultaneous Use"
    uint32_t queueFamilyIndex; 
    
    CommandBuffer(CommandBufferBlueprint& rBlueprint_, VkCommandBuffer vkHandle_);
    void recordCommands(); // Record commands present in commandsToRecord function

    VkCommandBuffer get() const;
};
