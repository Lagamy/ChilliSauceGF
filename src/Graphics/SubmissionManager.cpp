#include "SubmissionManager.h"
#include "Api.h"
#include "SubmissionBatchId.h"
#include "SubmissionBatch.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
PoolId SubmissionManager::addSubmition(const char* name_, SubmissionBatchId batchId_, VkCommandBuffer* pCmdBuffer_, size_t cmdBuffersCount_) 
{ 
    SubmissionBatch& rBatch = this->submissionBatches[batchId_.queueFamily].get(batchId_.poolId);
    PoolId submitionId = rBatch.submissionEntries.add(name_);
    VkSubmitInfo& rSubmitInfo = rBatch.submissionEntries.back(); 
    
    rSubmitInfo = {}; 
	rSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 		
	rSubmitInfo.waitSemaphoreCount = 0;
		 
	rSubmitInfo.commandBufferCount = cmdBuffersCount_; 
	rSubmitInfo.pCommandBuffers = pCmdBuffer_;
	rSubmitInfo.signalSemaphoreCount = 0; 

	rBatch.submissionWaitSemaphores.emplace_back(); 
	rBatch.submissionWaitStages.emplace_back(); 
	rBatch.submissionSignalSemaphores.emplace_back(); 
    return submitionId;
}; 


void SubmissionManager::submitToGraphicsQueue(SubmissionBatchId batch_, VkFence signalFence_)
{
	if(batch_.queueFamily != GRAPHICS)
	{
		std::stringstream errorMessageStream; 
		errorMessageStream << "Submission Manager: Can't pass " << queueToName[batch_.queueFamily] << " Queue batch named " << this->submissionBatches[batch_.queueFamily].getName(batch_.poolId) << " to Graphics Queue. \n";  
		throw std::runtime_error(errorMessageStream.str()); 
	}
    SubmissionBatch& rSubmitionBatch = this->submissionBatches[batch_.queueFamily].get(batch_.poolId);
    VkResult result = vkQueueSubmit(getMainDevice().queues.graphicsQueue, rSubmitionBatch.submissionEntries.size(), rSubmitionBatch.submissionEntries.data(), signalFence_);  
	if(result != VK_SUCCESS)
	{
		std::stringstream errorMessageStream; 
		errorMessageStream << "Submission Manager: Failed to submit batch named " << this->submissionBatches[batch_.queueFamily].getName(batch_.poolId) << " to Graphics Queue.\n";  
		throw std::runtime_error(errorMessageStream.str());
	}
    rSubmitionBatch.clear(); 
};

void SubmissionManager::submitToTransferQueue(SubmissionBatchId batch_, VkFence signalFence_)
{
	if(batch_.queueFamily != TRANSFER)
	{
		std::stringstream errorMessageStream; 
		errorMessageStream << "Submission Manager: Can't pass " << queueToName[batch_.queueFamily] << " Queue batch named " << this->submissionBatches[batch_.queueFamily].getName(batch_.poolId) << " to Transfer Queue. \n";  
		throw std::runtime_error(errorMessageStream.str()); 
	}
    SubmissionBatch& rSubmitionBatch = this->submissionBatches[batch_.queueFamily].get(batch_.poolId);
    VkResult result = vkQueueSubmit(getMainDevice().queues.graphicsQueue, rSubmitionBatch.submissionEntries.size(), rSubmitionBatch.submissionEntries.data(), signalFence_);  
	if(result != VK_SUCCESS)
	{
		std::stringstream errorMessageStream; 
		errorMessageStream << "Submission Manager: Failed to submit batch named " << this->submissionBatches[batch_.queueFamily].getName(batch_.poolId) << " to Transfer Queue.\n";  
		throw std::runtime_error(errorMessageStream.str());
	}
    rSubmitionBatch.clear(); 
}

void SubmissionManager::submitToComputeQueue(SubmissionBatchId batch_, VkFence signalFence_)
{
	if(batch_.queueFamily != COMPUTE)
	{
		std::stringstream errorMessageStream; 
		errorMessageStream << "Submission Manager: Can't pass " << queueToName[batch_.queueFamily] << " Queue batch named " << this->submissionBatches[batch_.queueFamily].getName(batch_.poolId) << " to Compute Queue. \n";  
		throw std::runtime_error(errorMessageStream.str()); 
	}
    SubmissionBatch& rSubmitionBatch = this->submissionBatches[batch_.queueFamily].get(batch_.poolId);
    VkResult result = vkQueueSubmit(getMainDevice().queues.graphicsQueue, rSubmitionBatch.submissionEntries.size(), rSubmitionBatch.submissionEntries.data(), signalFence_);  
	if(result != VK_SUCCESS)
	{
		std::stringstream errorMessageStream; 
		errorMessageStream << "Submission Manager: Failed to submit batch named " << this->submissionBatches[batch_.queueFamily].getName(batch_.poolId) << " to Compute Queue.\n";  
		throw std::runtime_error(errorMessageStream.str());
	}
    rSubmitionBatch.clear(); 
}

void SubmissionManager::addWaitSemaphore(SubmissionBatchId batchId_, PoolId submitionId_, VkSemaphore waitSemaphore_, VkPipelineStageFlags pipelineStage_)  
{
	SubmissionBatch& rBatch = this->submissionBatches[batchId_.queueFamily].get(batchId_.poolId); 
	rBatch.submissionWaitSemaphores[submitionId_.id].emplace_back(waitSemaphore_); 
	rBatch.submissionWaitStages[submitionId_.id].emplace_back(pipelineStage_); 
	VkSubmitInfo& rSubmission = rBatch.submissionEntries.get(submitionId_); 
	// Due to vector realoc when 
	rSubmission.pWaitSemaphores = rBatch.submissionWaitSemaphores[submitionId_.id].data();
	rSubmission.waitSemaphoreCount++;
	rSubmission.pWaitDstStageMask = rBatch.submissionWaitStages[submitionId_.id].data(); 
}

void SubmissionManager::addSignalSemaphore(SubmissionBatchId batchId_, PoolId submitionId_, VkSemaphore signalSemaphore_)
{
	SubmissionBatch& rBatch = this->submissionBatches[batchId_.queueFamily].get(batchId_.poolId);
	rBatch.submissionEntries.isPoolIdValid(submitionId_); 
	rBatch.submissionSignalSemaphores[submitionId_.id].emplace_back(signalSemaphore_);
	VkSubmitInfo& rSubmission = rBatch.submissionEntries.get(submitionId_); 
	rSubmission.pSignalSemaphores = rBatch.submissionSignalSemaphores[submitionId_.id].data();
	rSubmission.signalSemaphoreCount++;
}
}