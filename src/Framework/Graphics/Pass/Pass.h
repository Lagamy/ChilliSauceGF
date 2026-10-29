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
    /* ONESHOT passes only: FRAME passes signal frameFenceIds, owned and picked by the framework. */
    PoolId signalFenceId = UninitializedPoolId;
    SyncRetrivalFunc dynamicSignalFenceFunc;
    bool dynamicSignalFence = false;
    bool enabled = false;
    /* FRAME passes only: one fence per frame in flight, signaled by the pass's submission of that frame. */
    std::vector<PoolId> frameFenceIds;


    Pass(const char* name_, PoolId signalFenceId_);
    Pass(const char* name_, SyncRetrivalFunc signalFenceRetrivalFunc_); 
    Pass(const char* name_); 
    uint32_t addTask(const char* name_, CmdBufferFunc cmdBufferFunc_);
};
} 