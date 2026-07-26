#pragma once 
#include "Task.h"
#include "Fence.h"
#include "PoolId.h"
#include "Utilities.h"

namespace Graphics
{
struct Pass 
{
    std::string name;
    std::vector<Task> tasks;
    PoolId signalFenceId = UninitializedPoolId;
    SyncRetrivalFunc dynamicSignalFenceFunc; 
    PoolId submissionBatchId;
    bool enabled = false;


    Pass(const char* name_, PoolId signalFenceId_);
    Pass(const char* name_, SyncRetrivalFunc signalFenceRetrivalFunc_); 
    Pass(const char* name_); 
    uint32_t addTask(const char* name_, CmdBufferFunc cmdBufferFunc_);
};
} 