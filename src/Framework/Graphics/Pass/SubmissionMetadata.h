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

    std::vector<DynamicSemaphoreRef> dynamicWaitSemRefs; 
    std::vector<DynamicSemaphoreRef> dynamicSignalSemRefs; 

    std::vector<uint32_t> cmdIds; // TODO: make per frame tasks retrive cmds each frame    
};
}