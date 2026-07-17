#pragma once 
#include "Task.h"
#include "Fence.h"
#include "PoolId.h"

namespace Graphics
{
struct Pass 
{
    std::string name; 
    std::vector<Task> tasks;
    VkFence signalFence;

    Pass(const char* name_, VkFence signalFence_);
    uint32_t addSubmission(const char* name_, CmdBufferFunc cmdBufferFunc_);
};
} 