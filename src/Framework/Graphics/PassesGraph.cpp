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
#include <algorithm>
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

uint32_t PassesGraph::addTaskToPass(const char* name_, PassId passId_, PoolId commandListId_)
{
    uint32_t taskId = getPass(passId_).addTask(name_, commandListId_); 
    this->registerTaskCmdBuffers(passId_, taskId);
    return taskId;
}

void PassesGraph::setTaskCommandList(PassId passId_, uint32_t taskId_, PoolId commandListId_)
{
    getPass(passId_).tasks[taskId_].commandListId = commandListId_;
}

/* The recorder carries only the ids of its task: the list is looked up in record, so it can be switched without touching the pools. */
void PassesGraph::record(const CmdRecorder& rRecorder_, VkCommandBuffer& cmdBuffer_)
{
    getCommandList(getPass(rRecorder_.passId).tasks[rRecorder_.taskId].commandListId).record(cmdBuffer_);
}

void PassesGraph::registerTaskCmdBuffers(PassId passId_, uint32_t taskId_)
{
    Task& rTask = getPass(passId_).tasks[taskId_];
    CmdRecorder recorder = {passId_, taskId_};
    // add CMD buffer to the corresponding Pool(add it, not init it)  
    
    if(passId_.cmdLifetime == ONESHOT)
    {
        CmdBuffers& rCommandBuffers = Globals::renderer.oneShotCommandPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
        rTask.cmdIds.emplace_back(rCommandBuffers.commandsToRecord.size()); 
        rCommandBuffers.commandsToRecord.emplace_back(recorder);
        rCommandBuffers.buffersToEnabled.emplace_back(UninitializedId); 
    }
    else
    {
        for(auto& rFrameResource : Globals::renderer.framesResources)
        {
            CmdBuffers& rCommandBuffers = rFrameResource.frameCmdPools.getPoolByQueue(passId_.queueFamily).commandBuffers; 
            rTask.cmdIds.emplace_back(rCommandBuffers.commandsToRecord.size());
            rCommandBuffers.commandsToRecord.emplace_back(recorder); 
            rCommandBuffers.buffersToEnabled.emplace_back(UninitializedId); 
        }
    }
}

/*
    A ONESHOT command buffer is recorded and submitted once, so a draw that has to be skipped inside it would be lost for good. The pass is held until every descriptor and mesh its command lists use is loaded and settled, and holding re-marks them as used every frame so they can't be unloaded meanwhile.
    FRAME passes are not held: they are re-recorded every frame and skip single draws (drawMeshIndexed).
*/
bool PassesGraph::areResourcesReady(PassId passId_)
{
    bool ready = true;
    for(const auto& rTask : this->getPass(passId_).tasks)
    {
        for(PoolId meshId : getCommandList(rTask.commandListId).collectMeshUses())
        {
            Mesh& rMesh = getMesh(meshId);
            rMesh.markUsed();
            if(!rMesh.isSettled())
            {
                ready = false;
            }
        }
        for(const BoundDescriptor& rUse : getCommandList(rTask.commandListId).collectDescriptorUses())
        {
            DescriptorSet& rSet = getDescriptorSet(rUse.setId);
            rSet.markUsed(rUse.descriptorId);
            if(!rSet.isDescriptorSettled(rUse.descriptorId))
            {
                ready = false;
            }
        }
    }
    return ready;
}

void PassesGraph::releaseHeldPasses()
{
    std::vector<PassId> held = std::move(this->heldPasses);
    this->heldPasses.clear();
    for(const PassId& rPassId : held)
    {
        this->enablePass(rPassId);
    }
}

void PassesGraph::enablePass(PassId passId_)
{
    Pass& rPass = this->getPass(passId_);
    if(!rPass.enabled)
    {
        if(passId_.cmdLifetime == ONESHOT && !this->areResourcesReady(passId_))
        {
            bool alreadyHeld = std::any_of(this->heldPasses.begin(), this->heldPasses.end(), [&](const PassId& rHeld_)
            {
                return rHeld_.cmdLifetime == passId_.cmdLifetime && rHeld_.queueFamily == passId_.queueFamily && rHeld_.id == passId_.id;
            });
            if(!alreadyHeld)
            {
                this->heldPasses.push_back(passId_);
            }
            return;
        }

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

/*
    Vulkan rule: a buffer may not be destroyed and a descriptor binding may not be rewritten while a pending command buffer uses it (VUID-vkDestroyBuffer-buffer-00922, VUID-vkUpdateDescriptorSets-None-03047). A submit is no longer pending once its fence signaled.
    A use recorded in frame N was submitted by the FRAME passes of frame N, whose fences waitForFrame(N % framesAtFlightCount) waits on. Renderer::draw waits on its own slot at the start of every frame, so a use that is framesAtFlightCount frames old or older was waited on already.
*/
void PassesGraph::waitForUse(uint64_t lastUsedFrame_)
{
    if(lastUsedFrame_ == NeverUsedFrame)
    {
        return;
    }
    uint64_t framesAtFlight = Globals::renderer.framesAtFlightCount;
    if(Globals::renderer.frameNumber - lastUsedFrame_ < framesAtFlight)
    {
        this->waitForFrame(static_cast<uint32_t>(lastUsedFrame_ % framesAtFlight));
    }
}

/*
    A ONESHOT submit has no frame fence, so resolveSync_SubmitToGPU gives every one that uses meshes or descriptors a tracking fence (an empty submit after it: a fence signals once everything submitted to the queue before it finished) and records what it uses.
    Only the submits that use the resource are waited on, the rest of the device keeps running.
*/
void PassesGraph::waitForMeshUse(PoolId meshId_)
{
    this->waitForUse(getMesh(meshId_).residency.lastUsedFrame);
    for(const OneShotFlight& rFlight : this->oneShotsInFlight)
    {
        if(std::find(rFlight.meshIds.begin(), rFlight.meshIds.end(), meshId_) != rFlight.meshIds.end())
        {
            this->waitForOneShotFlight(rFlight);
        }
    }
}

/*
    A host write into a CPU_SHARED copy needs every submit that reads the copy to be finished (write after read). A FRAME pass is covered by waitForFrame, a ONESHOT submit by its tracking fence.
    The flight lists the entry, not the copy, so it is waited on whichever copy it was recorded with.
*/
void PassesGraph::waitForEntryUse(const MemoryEntryId& rEntryId_)
{
    for(const OneShotFlight& rFlight : this->oneShotsInFlight)
    {
        for(const MemoryEntryId& rUsed : rFlight.entryIds)
        {
            if(rUsed.allocatorType == rEntryId_.allocatorType && rUsed.id == rEntryId_.id)
            {
                this->waitForOneShotFlight(rFlight);
                break;
            }
        }
    }
}

void PassesGraph::waitForOneShots()
{
    for(const OneShotFlight& rFlight : this->oneShotsInFlight)
    {
        this->waitForOneShotFlight(rFlight);
    }
}

void PassesGraph::waitForOneShotFlight(const OneShotFlight& rFlight_)
{
    VkFence fence = getFence(rFlight_.fenceId);
    vkWaitForFences(getMainDevice().logicalDevice, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
}

void PassesGraph::waitForDescriptorUse(PoolId setId_, PoolId descriptorId_)
{
    this->waitForUse(getDescriptorSet(setId_).getLastUsedFrame(descriptorId_));
    for(const OneShotFlight& rFlight : this->oneShotsInFlight)
    {
        for(const BoundDescriptor& rUse : rFlight.descriptors)
        {
            if(rUse.setId == setId_ && rUse.descriptorId == descriptorId_)
            {
                this->waitForOneShotFlight(rFlight);
                break;
            }
        }
    }
}

void PassesGraph::trackOneShot(const SubmissionBatch& rBatch_)
{
    OneShotFlight flight;
    for(const auto& rTask : this->getPass(rBatch_.passId).tasks)
    {
        const CommandList& rList = getCommandList(rTask.commandListId);
        for(PoolId meshId : rList.collectMeshUses())
        {
            flight.meshIds.push_back(meshId);
            flight.entryIds.push_back(getMesh(meshId).vbMemoryUploadId);
        }
        for(const BoundDescriptor& rUse : rList.collectDescriptorUses())
        {
            flight.descriptors.push_back(rUse);
            flight.entryIds.push_back(getDescriptorSet(rUse.setId).getDescriptorEntryId(rUse.descriptorId));
        }
    }
    if(flight.meshIds.empty() && flight.descriptors.empty())
    {
        return;
    }

    if(this->freeOneShotFenceIds.empty())
    {
        flight.fenceId = addFence(false);
    }
    else
    {
        flight.fenceId = this->freeOneShotFenceIds.back();
        this->freeOneShotFenceIds.pop_back();
    }
    vkQueueSubmit(getQueue(rBatch_.queueFamily), 0, nullptr, getFence(flight.fenceId));
    this->oneShotsInFlight.push_back(std::move(flight));
}

void PassesGraph::retireOneShots()
{
    for(uint32_t i = 0; i < this->oneShotsInFlight.size();)
    {
        VkFence fence = getFence(this->oneShotsInFlight[i].fenceId);
        if(vkGetFenceStatus(getMainDevice().logicalDevice, fence) == VK_SUCCESS)
        {
            vkResetFences(getMainDevice().logicalDevice, 1, &fence);
            this->freeOneShotFenceIds.push_back(this->oneShotsInFlight[i].fenceId);
            this->oneShotsInFlight.erase(this->oneShotsInFlight.begin() + i);
        }
        else
        {
            i++;
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
                rBatch.passId = rPassId;
                
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

            /*
                Queues don't order work against each other, so with separate queue families the first submit of the batch also waits for the queues that submitted before it,
                and the last one signals this queue's done semaphore (see QueueTransferSync). The batch's own semaphores stay untouched, they are reused by later frames.
            */
            rMetadata.submitWaitSemaphores = rMetadata.waitSemaphores;
            rMetadata.submitWaitStages = rMetadata.waitStages;
            rMetadata.submitSignalSemaphores = rMetadata.signalSemaphores;
            if(submissionId == 0)
            {
                getMemoryManager().queueTransferSync.addWaits(rMetadata.submitWaitSemaphores, rMetadata.submitWaitStages);
            }
            if(submissionId + 1 == rBatch.submitInfos.size())
            {
                getMemoryManager().queueTransferSync.addSignal(rBatch.queueFamily, rMetadata.submitSignalSemaphores);
            }

            rSubmitInfo = {};
            rSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            rSubmitInfo.pWaitSemaphores = rMetadata.submitWaitSemaphores.data(); 
            rSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(rMetadata.submitWaitSemaphores.size()); 
            rSubmitInfo.pWaitDstStageMask = rMetadata.submitWaitStages.data(); 
            rSubmitInfo.pSignalSemaphores = rMetadata.submitSignalSemaphores.data(); 
            rSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(rMetadata.submitSignalSemaphores.size());
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
        if(rBatch.lifetime == ONESHOT)
        {
            this->trackOneShot(rBatch);
        }
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

       