#include "Pass.h"

namespace Graphics
{
Pass::Pass(const char* name_) : name(name_), dynamicSignalFence(false) {};
Pass::Pass(const char* name_, PoolId signalFenceId_) : name(name_), signalFenceId(signalFenceId_), dynamicSignalFence(false) {}; 
Pass::Pass(const char* name_, SyncRetrivalFunc signalFenceRetrivalFunc_) : name(name_), dynamicSignalFenceFunc(signalFenceRetrivalFunc_), dynamicSignalFence(true) {};  
uint32_t Pass::addTask(const char* name_, CmdBufferFunc cmdBufferFunc_)
{
    this->tasks.emplace_back(name_, cmdBufferFunc_);
    return this->tasks.size() - 1; 
}
}