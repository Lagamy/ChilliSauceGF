#include "PassesGraph.h"
#include "Api.h"
#include "CmdBuffers.h"
#include "FrameResources.h"
#include "Globals.h"
#include "PassId.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "SubmissionSync.h"
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
        
        PoolId submissionId = this->submissionBatchesPerQueue[passId_.queueFamily].add(rPass.name.c_str(), static_cast<bool>(passId_.cmdLifetime), rPass.signalFenceId); 
        rPass.submissionBatchId = submissionId; 
        SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[passId_.queueFamily].get(submissionId);

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
                for(uint32_t i = 0; i < rFrameResources.size(); i++)
                {
                                        
                    CmdBuffers& rCommandBuffers = rFrameResources[i].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers;

                    for(const auto frameCmdId : rTask.cmdIds)
                    {
                        rCommandBuffers.buffersToEnabled[frameCmdId] = rCommandBuffers.enabled.size();
                        rCommandBuffers.enabled.emplace_back(frameCmdId);
                    }
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
                    CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                    rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rTask.cmdIds[0]]); 
                    rCommandBuffers.buffersToEnabled[rTask.cmdIds[0]] = UninitializedId; 
                }
                else
                {
                    std::vector<FrameResources>& rFrameResources = Globals::renderer.framesResources;
                    for(uint32_t i = 0; i < rFrameResources.size(); i++)
                    {
                                        
                        CmdBuffers& rCommandBuffers = rFrameResources[i].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
                        for(const auto& rCmdId : rTask.cmdIds)
                        {
                            rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rCmdId]); 
                            rCommandBuffers.buffersToEnabled[rCmdId] = UninitializedId;  
                        }
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
                    rSubmissionBatch.perSubmissionSync.emplace_back(); 
                    SubmissionSync& rSync = rSubmissionBatch.perSubmissionSync.back(); 

                    rSync.waitSemaphores.resize(rTask.waitSemaphoresIds.size());
                    for(uint32_t j = 0; j < rSync.waitSemaphores.size(); j++)
                    {   
                        if(rTask.waitSemaphoresIds[j] != UninitializedPoolId)
                        {
                           rSync.waitSemaphores[j] = getSemaphore(rTask.waitSemaphoresIds[j]); 
                        }
                    }
                    rSync.signalSemaphores.resize(rTask.signalSemaphoresIds.size()); 
                    for(uint32_t j = 0; j < rSync.signalSemaphores.size(); j++)
                    {
                        if(rTask.signalSemaphoresIds[j] != UninitializedPoolId)
                        {
                            rSync.signalSemaphores[j] = getSemaphore(rTask.signalSemaphoresIds[j]); 
                        }
                    }

                    // Add secondary buffer support later on.
                    VkSubmitInfo submitInfo = {}; 
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.pWaitSemaphores = rSync.waitSemaphores.data(); 
                    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(rSync.waitSemaphores.size()); 
                    submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
                    submitInfo.pSignalSemaphores = rSync.signalSemaphores.data(); 
                    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(rSync.signalSemaphores.size());
                    submitInfo.pCommandBuffers = &getCommandBuffer(rPassId.queueFamily, rPassId.cmdLifetime, rTask.cmdIds[0]);
                    submitInfo.commandBufferCount = 1; 
                    
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
            SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[rPassId.queueFamily].get(rPass.submissionBatchId); 
            for(uint32_t i = 0; i < rPass.tasks.size(); i++)
            {
                for(const auto& rDynamicWaitRef : rPass.tasks[i].dynamicWaitSemaphoreRefs)
                {
                    rSubmissionBatch.perSubmissionSync[i].waitSemaphores[rDynamicWaitRef.id] = getSemaphore(rDynamicWaitRef.func());
                    // rSubmissionBatch.submissions[i].pWaitSemaphores[rDynamicWaitRef.id] = 
                }
            
                for(const auto& rDynamicSignalRef : rPass.tasks[i].dynamicSignalSemaphoreRefs)
                {
                    rSubmissionBatch.perSubmissionSync[i].signalSemaphores[rDynamicSignalRef.id] = getSemaphore(rDynamicSignalRef.func());
                }
            
            }
        }
    }


    for(uint8_t i = 0; i < 3; i++)
    {
        for(const auto& rPass: this->passesPerCmdType[FRAME].passesPerQueue[i])
        {

            SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[i].get(rPass.submissionBatchId); 
            for(const auto& rTask : rPass.tasks)
            {
                rSubmissionBatch.submissions[i].pCommandBuffers = &getCommandBuffer(i, FRAME, rTask.cmdIds[getCurrentFrameIndex()]);
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
					CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(i).commandBuffers; 
                    rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + rCommandBuffers.buffersToEnabled[rCmdBufferToDisable]); 
                    rCommandBuffers.buffersToEnabled[rCmdBufferToDisable] = UninitializedId; 
				}
				rSubmissionBatches.removeInternal(j); 
			}
		}
	}
}     
}
