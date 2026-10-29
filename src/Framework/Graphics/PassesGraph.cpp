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
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
PassId PassesGraph::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_)
{
    if(cmdType_ == FRAME)
    {
        throw std::runtime_error(std::format("Passes Graph: FRAME pass {} can't take a fence, its per frame fences are owned by the pass.", name_));
    }
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_, signalFenceId_);
    PassId id = {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1};
    this->passesOrder.emplace_back(id); 
    return id;
}

/*
    A fence only covers the submission it was passed to, so every FRAME pass gets its own fence per frame in flight (created signaled, the first wait must not block).
    Without them a frame with several passes would only be waited on for one of them.
*/
PassId PassesGraph::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_);
    if(cmdType_ == FRAME)
    {
        Pass& rPass = this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].back();
        rPass.frameFenceIds.reserve(Globals::renderer.framesAtFlightCount);
        for(uint32_t frame = 0; frame < Globals::renderer.framesAtFlightCount; frame++)
        {
            rPass.frameFenceIds.emplace_back(addFence(true));
        }
    }
    PassId id = {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1};
    this->passesOrder.emplace_back(id); 
    return id;
}

PassId PassesGraph::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc singalFenceFunc_)
{
    if(cmdType_ == FRAME)
    {
        throw std::runtime_error(std::format("Passes Graph: FRAME pass {} can't take a fence, its per frame fences are owned by the pass.", name_));
    }
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
        for(const auto& rTask : rPass.tasks)
        {
                        // Enable Cmds.
            if(passId_.cmdLifetime == ONESHOT)
            {
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
                std::vector<FrameResources>& rFrameResources = Globals::renderer.framesResources;
                for(uint32_t frameId = 0; frameId < rFrameResources.size(); frameId++)
                {
                    CmdBuffers& rCommandBuffers = rFrameResources[frameId].frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers;
                    uint32_t enabledId = rCommandBuffers.buffersToEnabled[rTask.cmdIds[frameId]];
                    rCommandBuffers.enabled.erase(rCommandBuffers.enabled.begin() + enabledId);
                    rCommandBuffers.buffersToEnabled[rTask.cmdIds[frameId]] = UninitializedId;

                    /* erase shifted every later entry down by one, so its stored position must follow */
                    for(uint32_t i = enabledId; i < rCommandBuffers.enabled.size(); i++)
                    {
                        rCommandBuffers.buffersToEnabled[rCommandBuffers.enabled[i]] = i;
                    }
                }
            }
            rPass.enabled = false;
            this->enabledPassesCount--;
            this->orderDirty = true;
        }
    }
}

/*
    Blocks until every FRAME pass has finished its submission of frame frameIndex_, which is what makes that frame's command pools and CPU_SHARED memory safe for the CPU to touch.
    Waiting doesn't reset the fences, resolveSync_SubmitToGPU does that when it submits. Must not be called between a pass's fence reset and its submit.
*/
void PassesGraph::waitForFrame(uint32_t frameIndex_)
{
    for(const auto& rPassesPerQueue : this->passesPerCmdType[FRAME].passesPerQueue)
    {
        for(const auto& rPass : rPassesPerQueue)
        {
            VkFence fence = getFence(rPass.frameFenceIds[frameIndex_]);
            vkWaitForFences(getMainDevice().logicalDevice, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
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
                rBatch.frameFenceIds = rPass.frameFenceIds;

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
                    
                    if(!rMetadata.dynamicSignalSemRefs.empty() || !rMetadata.dynamicWaitSemRefs.empty())
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
        if(rBatch.lifetime == FRAME)
        {
            signalFence = getFence(rBatch.frameFenceIds[getCurrentFrameIndex()]);
        }
        else
        {
            if(rBatch.dynamicSignalFence)
            {
                rBatch.signalFenceId = rBatch.dynamicSignalFenceFunc();
            }

            if(rBatch.signalFenceId != UninitializedPoolId)
            {
                signalFence = getFence(rBatch.signalFenceId);
            }
        }

        for(const auto& rSubmissionId : rBatch.submissionsWithDynamicSync)
        {
            this->resolveDynamicSync(rBatch, rBatch.perSubmissionMetadata[rSubmissionId]);
        }

        for(uint32_t submissionId = 0; submissionId < rBatch.submitInfos.size(); submissionId++)
        {
            VkSubmitInfo& rSubmitInfo = rBatch.submitInfos[submissionId];
            SubmissionMetadata& rMetadata = rBatch.perSubmissionMetadata[submissionId];

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

        /*
            A fence passed to vkQueueSubmit has to be unsignaled with no pending use, so a frame fence is reset right before the submit that signals it, not at the start of the frame.
            A pass that is disabled this frame never reaches this point, so its fence stays signaled and waitForFrame doesn't hang on it.
        */
        if(rBatch.lifetime == FRAME)
        {
            vkResetFences(getMainDevice().logicalDevice, 1, &signalFence);
        }
	    vkQueueSubmit(getQueue(rBatch.queueFamily), rBatch.submitInfos.size(), rBatch.submitInfos.data(), signalFence);
        if(rBatch.lifetime == ONESHOT) // cmdBuffers were already disabled when recorded, remove this submissionBatch from list
	    {
		    this->submissionBatches.erase(this->submissionBatches.begin() + batchId); 
	    }
        else 
        {
            batchId++; 
        }
    }
}     
}

       