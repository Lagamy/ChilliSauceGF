#include "SubmitionManager.h"
#include "Api.h"
#include "SubmitionBatch.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
PoolId SubmitionManager::addGraphicsSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_) 
{ 
    SubmitionBatch& batch = this->graphicsSubmitionBatches.get(batchId_);
    PoolId submitionId = batch.submitionEntries.add(name_);
    VkSubmitInfo& rSubmitInfo = batch.submitionEntries.back(); 
    
    rSubmitInfo = {}; 
	rSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 		
	rSubmitInfo.waitSemaphoreCount = waitSemaphoresCount_; 
	rSubmitInfo.pWaitSemaphores = pWaitSemaphores_;
	rSubmitInfo.pWaitDstStageMask = pWaitSemToStages_;
		 
	rSubmitInfo.commandBufferCount = cmdBufferCount_; 
	rSubmitInfo.pCommandBuffers = pCmdBuffer_;
	rSubmitInfo.signalSemaphoreCount = signalSemaphoresCount_; 
	rSubmitInfo.pSignalSemaphores = pSignalSemaphores_;
    return submitionId;
}; 

PoolId SubmitionManager::addTransferSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_) 
{ 
    SubmitionBatch& batch = this->transferSubmitionBatches.get(batchId_);
    PoolId submitionId = batch.submitionEntries.add(name_);
    VkSubmitInfo& rSubmitInfo = batch.submitionEntries.back(); 
    
    rSubmitInfo = {}; 
	rSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 		
	rSubmitInfo.waitSemaphoreCount = waitSemaphoresCount_; 
	rSubmitInfo.pWaitSemaphores = pWaitSemaphores_;
	rSubmitInfo.pWaitDstStageMask = pWaitSemToStages_;
		 
	rSubmitInfo.commandBufferCount = cmdBufferCount_; 
	rSubmitInfo.pCommandBuffers = pCmdBuffer_;
	rSubmitInfo.signalSemaphoreCount = signalSemaphoresCount_; 
	rSubmitInfo.pSignalSemaphores = pSignalSemaphores_;
    return submitionId;
}; 

PoolId SubmitionManager::addComputeSubmition(const char* name_, PoolId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBufferCount_, VkSemaphore* pWaitSemaphores_, size_t waitSemaphoresCount_, 
        VkPipelineStageFlags* pWaitSemToStages_, VkSemaphore* pSignalSemaphores_, size_t signalSemaphoresCount_) 
{ 
    SubmitionBatch& batch = this->computeSubmitionBatches.get(batchId_);
    PoolId submitionId = batch.submitionEntries.add(name_);
    VkSubmitInfo& rSubmitInfo = batch.submitionEntries.back(); 
    
    rSubmitInfo = {}; 
	rSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 		
	rSubmitInfo.waitSemaphoreCount = waitSemaphoresCount_; 
	rSubmitInfo.pWaitSemaphores = pWaitSemaphores_;
	rSubmitInfo.pWaitDstStageMask = pWaitSemToStages_;
		 
	rSubmitInfo.commandBufferCount = cmdBufferCount_; 
	rSubmitInfo.pCommandBuffers = pCmdBuffer_;
	rSubmitInfo.signalSemaphoreCount = signalSemaphoresCount_; 
	rSubmitInfo.pSignalSemaphores = pSignalSemaphores_;
    return submitionId;
}; 

void SubmitionManager::submitToGraphicsQueue(PoolId batch_, VkFence signalFence_)
{
    SubmitionBatch& rSubmitionBatch = this->graphicsSubmitionBatches.get(batch_);
    VkResult result = vkQueueSubmit(getMainDevice().queues.graphicsQueue, rSubmitionBatch.submitionEntries.size(), rSubmitionBatch.submitionEntries.data(), signalFence_);  
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("Triangle: Failed to submit renderFrame cmdBuffer to the Graphics Queue"); 
	}
    rSubmitionBatch.submitionEntries.clear(); 
};

void SubmitionManager::submitToTransferQueue(PoolId batch_, VkFence signalFence_)
{
    SubmitionBatch& rSubmitionBatch = this->transferSubmitionBatches.get(batch_);
    VkResult result = vkQueueSubmit(getMainDevice().queues.transferQueue, rSubmitionBatch.submitionEntries.size(), rSubmitionBatch.submitionEntries.data(), signalFence_);  
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("Triangle: Failed to submit renderFrame cmdBuffer to the Graphics Queue"); 
	}
    rSubmitionBatch.submitionEntries.clear(); 
} 

void SubmitionManager::submitToComputeQueue(PoolId batch_, VkFence signalFence_)
{
     SubmitionBatch& rSubmitionBatch = this->computeSubmitionBatches.get(batch_);
    VkResult result = vkQueueSubmit(getMainDevice().queues.computeQueue, rSubmitionBatch.submitionEntries.size(), rSubmitionBatch.submitionEntries.data(), signalFence_);  
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("Triangle: Failed to submit renderFrame cmdBuffer to the Graphics Queue"); 
	}
    rSubmitionBatch.submitionEntries.clear(); 
}
}