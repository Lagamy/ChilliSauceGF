#include "Task.h"

namespace Graphics
{
Task::Task(const char* name_, CmdBufferFunc cmdBufferFunc_) : name(name_), cmdBufferFunc(cmdBufferFunc_) {}; 

void Task::addWaitSemaphore(PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_)
{
    this->waitSemaphoresIds.emplace_back(waitSemaphoreId_); 
    this->waitStages.emplace_back(pipelineStage_);
}  

void Task::addSignalSemaphore(PoolId signalSemaphoreId_)
{
    this->signalSemaphoresIds.emplace_back(signalSemaphoreId_);
} 
}