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
    PoolId signalFenceId;
    PoolId submissionId; 
    bool enabled = false;  

    Pass(const char* name_, PoolId signalFenceId_);
    uint32_t addTask(const char* name_, CmdBufferFunc cmdBufferFunc_);
};
} 