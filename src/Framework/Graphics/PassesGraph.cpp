#include "PassesGraph.h"
#include "Api.h"
#include "CmdBuffers.h"
#include "FrameResources.h"
#include "Globals.h"
#include "PassId.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "SubmissionMetadata.h"
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
} 

void PassesGraph::addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_)
{
    Pass& rPass = getPass(passId_); 
    rPass.tasks[taskId_].addDynammicSignalSemaphore(semaphoreRetrivalFunc_); 
}

uint32_t PassesGraph::addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_)
{
    Pass& rPass = getPass(passId_);
    uint32_t taskId = rPass.addTask(name_, cmdBufferFunc_); 
    Task& rTask = rPass.tasks[taskId];  
    // add CMD buffer to the corresponding Pool(add it, not init it)  
    
    if(passId_.cmdLifetime == ONESHOT)
    {
        CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
        rTask.cmdIds.emplace_back(rCommandBuffers.commandsToRecord.size()); 
        rCommandBuffers.commandsToRecord.emplace_back(rTask.cmdBufferFunc);
        rCommandBuffers.buffersToEnabled.emplace_back(UninitializedId); 
    }
    else
    {
        for(auto& rFrameResource : Globals::renderer.framesResources)
        {
            CmdBuffers& rCommandBuffers = rFrameResource.frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
            rTask.cmdIds.emplace_back(rCommandBuffers.commandsToRecord.size());
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
        rPass.submissionBatchId = this->submissionBatches.size(); 
        this->submissionBatches.emplace_back(rPass.name, passId_.cmdLifetime, passId_.queueFamily, rPass.signalFenceId); 
        SubmissionBatch& rSubmissionBatch = this->submissionBatches.back();

        for(const auto& rTask : rPass.tasks)
        {
                        // Enable Cmds. 
            if(passId_.cmdLifetime == ONESHOT)
            {
                rSubmissionBatch.cmdBuffersToDisable.emplace_back(rTask.cmdIds[0]);
                CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                rCommandBuffers.buffersToEnabled[rTask.cmdIds[0]] = rCommandBuffers.enabled.size();
                rCommandBuffers.enabled.emplace_back(rTask.cmdIds[0]);
            }
            else
            {
                std::vector<FrameResources>& rFrameResources = Globals::renderer.framesResources;
                for(uint32_t frameId = 0; frameId < rFrameResources.size(); frameId++)
                {
                                        
                    CmdBuffers& rCommandBuffers = rFrameResources[frameId].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers;

                    rCommandBuffers.buffersToEnabled[rTask.cmdIds[frameId]] = rCommandBuffers.enabled.size();
                    rCommandBuffers.enabled.emplace_back(rTask.cmdIds[frameId]);
                }
            }
        }
        this->enabledPassesCount++;
        rPass.enabled = true;
        this->orderDirty = true; 
    }
} 

void PassesGraph::disableFramePass(PassId passId_)
{
    if(passId_.cmdLifetime == FRAME)
    {
        Pass& rPass = getPass(passId_); 
        if(rPass.enabled)
        {
            for(const auto& rTask : rPass.tasks)
            {
                if(passId_.cmdLifetime == ONESHOT)
                {
                    CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                    rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rTask.cmdIds[0]]); 
                    rCommandBuffers.buffersToEnabled[rTask.cmdIds[0]] = UninitializedId; 
                }
                else
                {
                    std::vector<FrameResources>& rFrameResources = Globals::renderer.framesResources;
                    for(uint32_t frameId = 0; frameId < rFrameResources.size(); frameId++)
                    {
                                        
                        CmdBuffers& rCommandBuffers = rFrameResources[frameId].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                        rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rTask.cmdIds[frameId]]); 
                        rCommandBuffers.buffersToEnabled[rTask.cmdIds[frameId]] = UninitializedId;  
                    }
                }
            }
            this->enabledPassesCount++;
            this->orderDirty = true; 
        }
    }
}

void PassesGraph::compileIfDirty()
{
    if(this->orderDirty)
    {
        this->submissionBatches.clear();
        this->submissionBatches.reserve(this->enabledPassesCount);
        for(const auto& rPassId : this->passesOrder)
        {
            Pass& rPass = getPass(rPassId);
            if(rPass.enabled)
            {
                this->submissionBatches.emplace_back(rPass.name, rPassId.cmdLifetime, rPassId.queueFamily, rPass.signalFenceId);  
                SubmissionBatch& rBatch = this->submissionBatches.back(); 
                
                if(rPass.dynamicSignalFence)
                {
                    rBatch.dynamicSignalFence = true; 
                    rBatch.dynamicSignalFenceFunc = rPass.dynamicSignalFenceFunc;
                }
                else
                {
                    rBatch.signalFenceId = rPass.signalFenceId; 
                }

                rBatch.perSubmissionMetadata.reserve(rPass.tasks.size());

                for(const auto& rTask : rPass.tasks)
                {
                    // Add submission
                    // Add secondary buffer support later on.
                    rBatch.perSubmissionMetadata.emplace_back(); 
                    SubmissionMetadata& rMetadata = rBatch.perSubmissionMetadata.back(); 

                    // Retrieving Semaphores 
                    rMetadata.waitSemaphores.resize(rTask.waitSemaphoresIds.size());
                    for(uint32_t j = 0; j < rMetadata.waitSemaphores.size(); j++)
                    {   
                        if(rTask.waitSemaphoresIds[j] != UninitializedPoolId)
                        {
                           rMetadata.waitSemaphores[j] = getSemaphore(rTask.waitSemaphoresIds[j]); 
                        }
                    }
                    rMetadata.waitStages = rTask.waitStages;

                    rMetadata.signalSemaphores.resize(rTask.signalSemaphoresIds.size()); 
                    for(uint32_t j = 0; j < rMetadata.signalSemaphores.size(); j++)
                    {
                        if(rTask.signalSemaphoresIds[j] != UninitializedPoolId)
                        {
                            rMetadata.signalSemaphores[j] = getSemaphore(rTask.signalSemaphoresIds[j]); 
                        }
                    }

                    // Set dynamic Semaphores Refs  
                    rMetadata.dynamicWaitSemRefs = rTask.dynamicWaitSemaphoreRefs; 
                    rMetadata.dynamicSignalSemRefs = rTask.dynamicSignalSemaphoreRefs; 
                    
                    if(!rMetadata.dynamicSignalSemRefs.empty() || rMetadata.dynamicSignalSemRefs.empty())
                    {
                        rBatch.submissionsWithDynamicSync.emplace_back(rBatch.perSubmissionMetadata.size() - 1);
                    }

                    // Set cmd ids 
                    rMetadata.cmdIds = rTask.cmdIds;  
                }

                rBatch.submitInfos.resize(rBatch.perSubmissionMetadata.size());
                if(rPassId.cmdLifetime == ONESHOT)
                {
                    rPass.enabled = false;
                    this->enabledPassesCount--;
                } 
            }
        }
        this->orderDirty = false; 
    }
}

void PassesGraph::resolveDynamicSync(SubmissionBatch& rBatch_, SubmissionMetadata& rMetadata)
{
    // resolve Dynamic Wait Semaphores 
    for(const auto& rDynWaitSemRef : rMetadata.dynamicWaitSemRefs)
    {
        rMetadata.waitSemaphores[rDynWaitSemRef.id] = getSemaphore(rDynWaitSemRef.func());  
    }

    // resolve Dynamic Signal Semaphores
    for(const auto& rDynSignalSemRef : rMetadata.dynamicSignalSemRefs)
    {
        rMetadata.signalSemaphores[rDynSignalSemRef.id] = getSemaphore(rDynSignalSemRef.func());  
    }
}

void PassesGraph::resolveSync_SubmitToGPU()
{
    for(uint32_t batchId = 0; batchId < this->submissionBatches.size();)
	{
        SubmissionBatch& rBatch = this->submissionBatches[batchId]; 
        VkFence signalFence = VK_NULL_HANDLE; 
		if(rBatch.dynamicSignalFence)
        {
            rBatch.signalFenceId = rBatch.dynamicSignalFenceFunc(); 
        }

        if(rBatch.signalFenceId != UninitializedPoolId)
	    {
		    signalFence = getFence(rBatch.signalFenceId); 
	    }

        for(uint32_t submissionId = 0; submissionId < rBatch.submitInfos.size(); submissionId++)
        {
            VkSubmitInfo& rSubmitInfo = rBatch.submitInfos[submissionId]; 
            SubmissionMetadata& rMetadata = rBatch.perSubmissionMetadata[submissionId]; 
            this->resolveDynamicSync(rBatch, rMetadata); 

            rSubmitInfo = {}; 
            rSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            rSubmitInfo.pWaitSemaphores = rMetadata.waitSemaphores.data(); 
            rSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(rMetadata.waitSemaphores.size()); 
            rSubmitInfo.pWaitDstStageMask = rMetadata.waitStages.data(); 
            rSubmitInfo.pSignalSemaphores = rMetadata.signalSemaphores.data(); 
            rSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(rMetadata.signalSemaphores.size());
            if(rBatch.lifetime == ONESHOT)
            {
                rSubmitInfo.pCommandBuffers = &getCommandBuffer(rBatch.queueFamily, ONESHOT, rBatch.perSubmissionMetadata[submissionId].cmdIds[0]);
            }
            else 
            {
                rSubmitInfo.pCommandBuffers = &getCommandBuffer(rBatch.queueFamily, FRAME, rBatch.perSubmissionMetadata[submissionId].cmdIds[getCurrentFrameIndex()]);
            }
            rSubmitInfo.commandBufferCount = 1; 
        }
        
	    vkQueueSubmit(getQueue(rBatch.queueFamily), rBatch.submitInfos.size(), rBatch.submitInfos.data(), signalFence); 
        if(rBatch.lifetime == ONESHOT) // disable cmdBuffers, and remove this submissionBatch from list 
	    {
		    for(const auto& rCmdBufferToDisable : this->submissionBatches[batchId].cmdBuffersToDisable)
		    { 
			    CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(rBatch.queueFamily).commandBuffers; 
                rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rCmdBufferToDisable]); 
                rCommandBuffers.buffersToEnabled[rCmdBufferToDisable] = UninitializedId; 
		    }
		    this->submissionBatches.erase(this->submissionBatches.begin() + batchId); 
	    }
        else 
        {
            batchId++; 
        }
    }
}     
}

       