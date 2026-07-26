#include "PassesGraph.h"
#include "Api.h"
#include "CmdBuffersInPasses.h"
#include "FrameResources.h"
#include "Globals.h"
#include "PassId.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "Task.h"
#include "Utilities.h"
#include <cstddef>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
PassId PassesGraph::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_, signalFenceId_);
    PassId id = {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1};
    this->passesOrder.emplace_back(id); 
    return id;
}

PassId PassesGraph::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_);
    PassId id = {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1};
    this->passesOrder.emplace_back(id); 
    return id;
}

PassId PassesGraph::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc singalFenceFunc_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_, singalFenceFunc_);
    PassId id = {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1};
    this->passesOrder.emplace_back(id); 
    this->passesWithDynamicFence.emplace_back(id);
    return id;
}

Pass& PassesGraph::getPass(PassId passId_)
{
    return this->passesPerCmdType[passId_.cmdLifetime].passesPerQueue[passId_.queueFamily][passId_.id];
}

void PassesGraph::addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_)
{
    Pass& rPass = getPass(passId_); 
    rPass.tasks[taskId_].addDynamicWaitSemaphore(semaphoreRetrivalFunc_, pipelineStage_); 
    this->passesWithDynamicTasks.emplace_back(passId_);  
} 

void PassesGraph::addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_)
{
    Pass& rPass = getPass(passId_); 
    rPass.tasks[taskId_].addDynammicSignalSemaphore(semaphoreRetrivalFunc_); 
    this->passesWithDynamicTasks.emplace_back(passId_);  
}

uint32_t PassesGraph::addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_)
{
    Pass& rPass = getPass(passId_);
    uint32_t taskId = rPass.addTask(name_, cmdBufferFunc_); 
    Task& rTask = rPass.tasks[taskId];  
    // add CMD buffer to the corresponding Pool(add it, not init it)  
    
    if(passId_.cmdLifetime == ONESHOT)
    {
        CmdBuffersInPasses& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
        rTask.cmdId = rCommandBuffers.commandsToRecord.size(); 
        rCommandBuffers.commandsToRecord.emplace_back(rTask.cmdBufferFunc);
        rCommandBuffers.buffersToEnabled.emplace_back(UninitializedId); 
    }
    else
    {
        rTask.cmdId = getCurrentFrameResources().frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers.commandsToRecord.size(); // All frame resources are 1:1. So we can get Id by fetching size any of them 
        for(auto& rFrameResource : Globals::renderer.framesResources)
        {
            CmdBuffersInPasses& rCommandBuffers = rFrameResource.frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
            rCommandBuffers.commandsToRecord.emplace_back(rTask.cmdBufferFunc); 
            rCommandBuffers.buffersToEnabled.emplace_back(UninitializedId); 
        }
    }
    return taskId;
}

void PassesGraph::enablePass(PassId passId_)
{
    Pass& rPass = this->getPass(passId_);
    if(!rPass.enabled)
    {
        
        PoolId submissionId = this->submissionBatchesPerQueue[passId_.queueFamily].add(rPass.name.c_str(), static_cast<bool>(passId_.cmdLifetime), rPass.signalFenceId); 
        rPass.submissionBatchId = submissionId; 
        SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[passId_.queueFamily].get(submissionId);

        for(const auto& rTask : rPass.tasks)
        {
                        // Enable Cmds. 
            if(passId_.cmdLifetime == ONESHOT)
            {
                rSubmissionBatch.cmdBuffersToDisable.emplace_back(rTask.cmdId);
                CmdBuffersInPasses& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                rCommandBuffers.buffersToEnabled[rTask.cmdId] = rCommandBuffers.enabled.size();
                rCommandBuffers.enabled.emplace_back(rTask.cmdId);
            }
            else
            {
                std::vector<FrameResources>& rFrameResources = Globals::renderer.framesResources;
                for(uint32_t i = 0; i < rFrameResources.size(); i++)
                {
                                        
                    CmdBuffersInPasses& rCommandBuffers = rFrameResources[i].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                    rCommandBuffers.buffersToEnabled[rTask.cmdId] = rCommandBuffers.enabled.size();
                    rCommandBuffers.enabled.emplace_back(rTask.cmdId);
                }
            }
        }
    }
    rPass.enabled = true;
    this->orderDirty = true; 
} 

void PassesGraph::disableFramePass(PassId passId_)
{
    if(passId_.cmdLifetime == FRAME)
    {
        Pass& rPass = getPass(passId_); 
        if(rPass.enabled)
        {
            this->submissionBatchesPerQueue[passId_.queueFamily].remove(rPass.submissionBatchId); 
            rPass.submissionBatchId = UninitializedPoolId;
            for(const auto& rTask : rPass.tasks)
            {
                if(passId_.cmdLifetime == ONESHOT)
                {
                    CmdBuffersInPasses& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                    rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rTask.cmdId]); 
                    rCommandBuffers.buffersToEnabled[rTask.cmdId] = UninitializedId; 
                }
                else
                {
                    std::vector<FrameResources>& rFrameResources = Globals::renderer.framesResources;
                    for(uint32_t i = 0; i < rFrameResources.size(); i++)
                    {
                                        
                        CmdBuffersInPasses& rCommandBuffers = rFrameResources[i].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                        rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rTask.cmdId]); 
                        rCommandBuffers.buffersToEnabled[rTask.cmdId] = UninitializedId;  
                    }
                }
            }
        }
    }
    this->orderDirty = true; 
}

void PassesGraph::compileIfDirty()
{
    if(this->orderDirty)
    {
        this->submissionBatchesPerQueue[0].clear(); 
        this->submissionBatchesPerQueue[1].clear(); 
        this->submissionBatchesPerQueue[2].clear(); 
        for(const auto& rPassId : this->passesOrder)
        {
            Pass& rPass = getPass(rPassId);
            if(rPass.enabled)
            {
                PoolId lastSubmissionBatchId = this->submissionBatchesPerQueue[rPassId.queueFamily].add(rPass.name.c_str(), static_cast<bool>(rPassId.cmdLifetime), rPass.signalFenceId);  
                for(const auto& rTask : rPass.tasks)
                {
                    // Add submission
                    // Add secondary buffer support later on.
                    SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[rPassId.queueFamily].back(); 
                    rSubmissionBatch.waitSemaphores.emplace_back(); 
                    rSubmissionBatch.signalSemaphores.emplace_back();
                    std::vector<VkSemaphore>& rWaitSemaphores = rSubmissionBatch.waitSemaphores.back(); 
                    std::vector<VkSemaphore>& rSignalSemaphores = rSubmissionBatch.signalSemaphores.back(); 
                    

                    rWaitSemaphores.resize(rTask.waitSemaphoresIds.size());
                    for(uint32_t j = 0; j < rWaitSemaphores.size(); j++)
                    {   
                        if(rTask.waitSemaphoresIds[j] != UninitializedPoolId)
                        {
                            rWaitSemaphores[j] = getSemaphore(rTask.waitSemaphoresIds[j]); 
                        }
                    }
                    rSignalSemaphores.resize(rTask.signalSemaphoresIds.size()); 
                    for(uint32_t j = 0; j < rSignalSemaphores.size(); j++)
                    {
                        if(rTask.signalSemaphoresIds[j] != UninitializedPoolId)
                        {
                            rSignalSemaphores[j] = getSemaphore(rTask.signalSemaphoresIds[j]); 
                        }
                    }
                    
                    // Add secondary buffer support later on.
                    VkSubmitInfo submitInfo = {}; 
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.pCommandBuffers = &getCommandBuffer(rPassId.queueFamily, rPassId.cmdLifetime, rTask.cmdId);
                    submitInfo.commandBufferCount = 1;  
                    submitInfo.pWaitSemaphores = rWaitSemaphores.data(); 
                    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(rWaitSemaphores.size()); 
                    submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
                    submitInfo.pSignalSemaphores = rSignalSemaphores.data(); 
                    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(rSignalSemaphores.size());
                    
                    rSubmissionBatch.submissions.emplace_back(submitInfo);
                }
                rPass.submissionBatchId = lastSubmissionBatchId; 
                if(rPassId.cmdLifetime == ONESHOT) rPass.enabled = false; 
            }
        }
    }
    this->orderDirty = false; 
}

void PassesGraph::resolveDynamicSync()
{
    for(const auto& rPassId : this->passesWithDynamicFence)
    {
        Pass& rPass = this->getPass(rPassId); 
        if(rPass.enabled)
        {
            this->submissionBatchesPerQueue[rPassId.queueFamily].get(rPass.submissionBatchId).signalFenceId = rPass.dynamicSignalFenceFunc();
        }
    }

    for(const auto& rPassId : this->passesWithDynamicTasks)
    {
        Pass& rPass = this->getPass(rPassId);
        if(rPass.enabled)
        { 
            for(uint32_t i = 0; i < rPass.tasks.size(); i++)
            {
            
                SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[rPassId.queueFamily].get(rPass.submissionBatchId); 
                for(const auto& rDynamicWaitRef : rPass.tasks[i].dynamicWaitSemaphoreRefs)
                {
                    rSubmissionBatch.waitSemaphores[i][rDynamicWaitRef.id] = getSemaphore(rDynamicWaitRef.func());
                }
            
                for(const auto& rDynamicSignalRef : rPass.tasks[i].dynamicSignalSemaphoreRefs)
                {
                    rSubmissionBatch.signalSemaphores[i][rDynamicSignalRef.id] = getSemaphore(rDynamicSignalRef.func());
                }
            }
        }
    }
}

void PassesGraph::submitToGPU()
{
    for(uint8_t i = 0; i < this->submissionBatchesPerQueue.size(); i++)
	{
		Pool<SubmissionBatch>& rSubmissionBatches = this->submissionBatchesPerQueue[i]; 
		for(uint32_t j = 0; j < rSubmissionBatches.size(); j++)
		{
			VkFence signalFence = VK_NULL_HANDLE; 
			if(rSubmissionBatches.objects[j].signalFenceId != UninitializedPoolId)
			{
				signalFence = getFence(rSubmissionBatches.objects[j].signalFenceId); 
			}

			vkQueueSubmit(getQueue(i), rSubmissionBatches.objects[j].submissions.size(), rSubmissionBatches.objects[j].submissions.data(), signalFence); 
			if(rSubmissionBatches.objects[j].oneShot) // disable cmdBuffers, and remove this submissionBatch from list 
			{
				for(const auto& rCmdBufferToDisable : rSubmissionBatches.objects[j].cmdBuffersToDisable)
				{ 
					CmdBuffersInPasses& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(i).commandBuffers; 
                    rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rCmdBufferToDisable]); 
                    rCommandBuffers.buffersToEnabled[rCmdBufferToDisable] = UninitializedId; 
				}
				rSubmissionBatches.removeInternal(j); 
			}
		}
	}
}     
}
