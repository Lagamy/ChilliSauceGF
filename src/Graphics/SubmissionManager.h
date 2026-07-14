#pragma once 
#include "SubmissionBatch.h"
#include "SubmissionBatchId.h"
#include "Pool.h"
#include "Utilities.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <array>

namespace Graphics
{
struct SubmissionManager 
{
    std::array<Pool<SubmissionBatch>, 3> submissionBatches; 

    // I keep Queue family submition explicit for readability reasons. 
    void submitToGraphicsQueue(SubmissionBatchId batchId_, VkFence signalFence_);
    void submitToTransferQueue(SubmissionBatchId batchId_, VkFence signalFence_); 
    void submitToComputeQueue(SubmissionBatchId batchId_, VkFence signalFence_); 

    PoolId addSubmition(const char* name_, SubmissionBatchId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_);

    void addWaitSemaphore(SubmissionBatchId batchId_, PoolId submitionId_, VkSemaphore waitSemaphore_, VkPipelineStageFlags pipelineStage_);  
    void addSignalSemaphore(SubmissionBatchId batchId_, PoolId submitionId_, VkSemaphore signalSemaphore_); 
};
} 