#include "Pass.h"

namespace Graphics
{
Pass::Pass(const char* name_) : name(name_) {};
Pass::Pass(const char* name_, PoolId signalFenceId_) : name(name_), signalFenceId(signalFenceId_) {}; 
Pass::Pass(const char* name_, SyncRetrivalFunc signalFenceRetrivalFunc_) : name(name_), dynamicSignalFenceFunc(signalFenceRetrivalFunc_) {};  
uint32_t Pass::addTask(const char* name_, CmdBufferFunc cmdBufferFunc_)
{
    this->tasks.emplace_back(name_, cmdBufferFunc_);
    return this->tasks.size() - 1; 
}
}