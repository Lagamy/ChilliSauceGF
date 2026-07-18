#pragma once 
#include "PoolId.h"
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
    std::vector<PoolId> waitSemaphoresIds; 
    std::vector<VkPipelineStageFlags> waitStages; 
    std::vector<PoolId> signalSemaphoresIds; 
    bool alive = true; // One Shot 

    Task(const char* name_, CmdBufferFunc cmdBufferFunc_); 
    void addWaitSemaphore(PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_);  
    void addSignalSemaphore(PoolId signalSemaphoreId_); 
};
}