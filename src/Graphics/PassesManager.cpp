#include "PassesManager.h"
#include "Api.h"
#include "CmdBuffersInPasses.h"
#include "FrameResources.h"
#include "Globals.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "Task.h"
#include "Utilities.h"
#include <cstddef>
#include <vector>

namespace Graphics
{
PassId PassesManager::addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_, signalFenceId_);
    return {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1}; 
}

Pass& PassesManager::getPass(PassId passId_)
{
    return this->passesPerCmdType[passId_.cmdLifetime].passesPerQueue[passId_.queueFamily][passId_.id];
}


uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_)
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

void PassesManager::enablePass(PassId passId_)
{
    Pass& rPass = this->getPass(passId_);
    if(!rPass.enabled)
    {
        
        PoolId submissionId = this->submissionBatchesPerQueue[passId_.queueFamily].add(rPass.name.c_str(), static_cast<bool>(passId_.cmdLifetime), rPass.signalFenceId); 
        rPass.submissionId = submissionId; 
        SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[passId_.queueFamily].get(submissionId);
        for(const auto& rTask : rPass.tasks)
        {
            // Add submission
            // Add secondary buffer support later on.
            std::vector<VkSemaphore> waitSemaphores;
            waitSemaphores.resize(rTask.waitSemaphoresIds.size());
            for(uint32_t j = 0; j < waitSemaphores.size(); j++)
            {
                waitSemaphores[j] = getSemaphore(rTask.waitSemaphoresIds[j]); 
            }
            std::vector<VkSemaphore> signalSemaphores;
            for(uint32_t j = 0; j < signalSemaphores.size(); j++)
            {
                signalSemaphores[j] = getSemaphore(rTask.signalSemaphoresIds[j]); 
            }
            signalSemaphores.resize(rTask.signalSemaphoresIds.size()); 
 
            // Add secondary buffer support later on.
            VkSubmitInfo submitInfo = {}; 
            submitInfo.pCommandBuffers = &getCommandBuffer(passId_.queueFamily, FRAME, rTask.cmdId);
            submitInfo.commandBufferCount = 1;  
            submitInfo.pWaitSemaphores = waitSemaphores.data(); 
            submitInfo.waitSemaphoreCount = waitSemaphores.size(); 
            submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
            submitInfo.pSignalSemaphores = signalSemaphores.data(); 
            submitInfo.signalSemaphoreCount = signalSemaphores.size(); 
            rSubmissionBatch.submissions.emplace_back(submitInfo);
           

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
} 

void PassesManager::disablePass(PassId passId_)
{
    if(passId_.cmdLifetime == FRAME)
    {
        Pass& rPass = getPass(passId_); 
        if(rPass.enabled)
        {
            this->submissionBatchesPerQueue[passId_.queueFamily].remove(rPass.submissionId); 
            rPass.submissionId = UninitializedPoolId;
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
}
}