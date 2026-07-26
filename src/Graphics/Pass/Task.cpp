#include "Task.h"
#include "Utilities.h"

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

void Task::addDynamicWaitSemaphore(SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_)
{
    this->waitSemaphoresIds.emplace_back(UninitializedPoolId); 
    this->waitStages.emplace_back(pipelineStage_); 
    this->dynamicWaitSemaphoreRefs.emplace_back(semaphoreRetrivalFunc_, this->waitSemaphoresIds.size() - 1); 
}

void Task::addDynammicSignalSemaphore(SyncRetrivalFunc semaphoreRetrivalFunc_)
{
    this->signalSemaphoresIds.emplace_back(UninitializedPoolId); 
    this->dynamicSignalSemaphoreRefs.emplace_back(semaphoreRetrivalFunc_, this->signalSemaphoresIds.size() - 1);
}
}