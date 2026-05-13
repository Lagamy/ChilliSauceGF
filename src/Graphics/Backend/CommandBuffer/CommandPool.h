// Purelly runtime class, allocates memory for command buffers derived from user created command buffer blueprints(scripting), than initializes and records all of them. 
#pragma once 
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include "CommandBuffer.h"
#include "CommandBufferBlueprint.h"

struct CommandPool {
    VkCommandPool vkHandle = VK_NULL_HANDLE;
    VkCommandBufferLevel level;  // Primary - can only be run by queue. Secondary - can only be called in another command buffer(using vkCmdExecureCommands(cmBuffer)).
    uint32_t queueFamilyIndex; 


    std::vector<CommandBuffer> commandBuffers; // Gets created from blueprints. Each blueprint represent seperate rendering phase(0 -> Game, 1 -> Editor Objects, 3 -> ImGUI, etc)
    std::vector<CommandBufferBlueprint*> rBlueprints;  

    void create(VkCommandBufferLevel level_, uint32_t queueFamilyIndex_);
    void resetCMDBuffers(); // You need to rerecord buffers each draw, cause games are dynamic. An entity that uses distinct Graphics Pipeline might spawn, and invalidate previous recorded one. + Performance cost is negligable.  
    void dealocateCMDBuffers();
    void destroy();
    VkCommandPool get() const; 

    void allocateIfActiveCommandBuffers();

    CommandPool() = default;
};
