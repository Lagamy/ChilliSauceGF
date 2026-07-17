#pragma once 
#include <functional>
#include <string>
#include <vulkan/vulkan.h>

namespace Graphics
{
using CmdBufferFunc = std::function<void(VkCommandBuffer&)>;
struct Task
{
    std::string name; 
    CmdBufferFunc cmdBufferFunc; 
    size_t cmdId; // Set by pool upon creation 
    std::vector<VkSemaphore> waitSemaphores; 
    std::vector<VkPipelineStageFlags> waitStages; 
    std::vector<VkSemaphore> signalSemaphores; 
    bool alive = true; // One Shot 

    Task(const char* name_, CmdBufferFunc cmdBufferFunc_); 
    void addWaitSemaphore(VkSemaphore waitSemaphore_, VkPipelineStageFlags pipelineStage_);  
    void addSignalSemaphore(VkSemaphore signalSemaphore_); 
};
}