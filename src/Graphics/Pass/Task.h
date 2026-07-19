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
    uint32_t cmdId; 
    std::vector<PoolId> waitSemaphoresIds; 
    std::vector<VkPipelineStageFlags> waitStages; 
    std::vector<PoolId> signalSemaphoresIds; 

    Task(const char* name_, CmdBufferFunc cmdBufferFunc_); 
    void addWaitSemaphore(PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_);  
    void addSignalSemaphore(PoolId signalSemaphoreId_); 
};
}