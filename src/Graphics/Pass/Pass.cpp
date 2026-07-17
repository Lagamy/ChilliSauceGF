#include "Pass.h"

namespace Graphics
{
Pass::Pass(const char* name_, VkFence signalFence_) : name(name_), signalFence(signalFence_) {}; 
uint32_t Pass::addSubmission(const char* name_, CmdBufferFunc cmdBufferFunc_)
{
    this->tasks.emplace_back(name_, cmdBufferFunc_);
    return this->tasks.size() - 1; 
}
}