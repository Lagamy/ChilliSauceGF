#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


namespace Graphics
{
struct Task
{
    std::string name; 
    PoolId commandListId = UninitializedPoolId; // the CommandList replayed when the task is recorded, can be changed (setTaskCommandList)
    std::vector<uint32_t> cmdIds; // TODO: make per frame tasks retrive cmds each frame  

    std::vector<PoolId> waitSemaphoresIds; 
    std::vector<VkPipelineStageFlags> waitStages; 
    std::vector<PoolId> signalSemaphoresIds;
    bool requiresAllocationToProceed = false;
    std::vector<AllocatorTypeEnum> allocatorsToWaitOn;  

    std::vector<DynamicSemaphoreRef> dynamicWaitSemaphoreRefs;
    std::vector<DynamicSemaphoreRef> dynamicSignalSemaphoreRefs;
    
    Task(const char* name_, PoolId commandListId_);
    void addWaitSemaphore(PoolId waitSemaphoreId_, VkPipelineStageFlags pipelineStage_);  
    void addSignalSemaphore(PoolId signalSemaphoreId_);
    void addDynamicWaitSemaphore(SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_); 
    void addDynammicSignalSemaphore(SyncRetrivalFunc semaphoreRetrivalFunc_); 
};
}