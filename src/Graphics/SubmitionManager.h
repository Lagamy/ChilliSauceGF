#pragma once 
#include "SubmitionBatch.h"
#include "Pool.h"
#include <vulkan/vulkan.h>

namespace Graphics
{
struct SubmitionManager 
{
    Pool<SubmitionBatch> graphicsSubmitionBatches;
    Pool<SubmitionBatch> transferSubmitionBatches; 
    Pool<SubmitionBatch> computeSubmitionBatches;  

    void submitToGraphicsQueue(PoolId batchId_, VkFence signalFence_); 
    void submitToTransferQueue(PoolId batchId_, VkFence signalFence_); 
    void submitToComputeQueue(PoolId batchId_, VkFence signalFence_); 
    PoolId addGraphicsSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_); 
    PoolId addTransferSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_); 
    PoolId addComputeSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_); 
};
} 