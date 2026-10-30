#pragma once 
#include "Utilities.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics
{

struct SubmissionMetadata 
{
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages; 
    std::vector<VkSemaphore> signalSemaphores;

    /* What the VkSubmitInfo of the current submit points at: the semaphores above plus the cross-queue ones QueueTransferSync adds. Rebuilt on every submit. */
    std::vector<VkSemaphore> submitWaitSemaphores;
    std::vector<VkPipelineStageFlags> submitWaitStages;
    std::vector<VkSemaphore> submitSignalSemaphores;

    std::vector<DynamicSemaphoreRef> dynamicWaitSemRefs; 
    std::vector<DynamicSemaphoreRef> dynamicSignalSemRefs; 

    std::vector<uint32_t> cmdIds; // TODO: make per frame tasks retrive cmds each frame    
};
}