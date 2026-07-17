#include "Task.h"

namespace Graphics
{
Task::Task(const char* name_, CmdBufferFunc cmdBufferFunc_) : name(name_), cmdBufferFunc(cmdBufferFunc_) {}; 

void Task::addWaitSemaphore(VkSemaphore waitSemaphore_, VkPipelineStageFlags pipelineStage_)
{
    this->waitSemaphores.emplace_back(waitSemaphore_); 
    this->waitStages.emplace_back(pipelineStage_);
}  

void Task::addSignalSemaphore(VkSemaphore signalSemaphore_)
{
    this->signalSemaphores.emplace_back(signalSemaphore_);
} 
}