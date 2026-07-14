#pragma once 
#include "Pool.h"
#include <vulkan/vulkan.h> 

namespace Graphics
{
struct SubmissionBatch 
{
    Pool<VkSubmitInfo> submissionEntries;
    std::vector<std::vector<VkSemaphore>> submissionWaitSemaphores; 
    std::vector<std::vector<VkPipelineStageFlags>> submissionWaitStages; 
    std::vector<std::vector<VkSemaphore>> submissionSignalSemaphores; 

    void clear(); 
};
}