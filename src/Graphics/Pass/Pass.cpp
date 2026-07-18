#include "Pass.h"

namespace Graphics
{
Pass::Pass(const char* name_, PoolId signalFenceId_) : name(name_), signalFenceId(signalFenceId_) {}; 
uint32_t Pass::addTask(const char* name_, CmdBufferFunc cmdBufferFunc_)
{
    this->tasks.emplace_back(name_, cmdBufferFunc_);
    return this->tasks.size() - 1; 
}
}