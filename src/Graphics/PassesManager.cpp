#include "PassesManager.h"
#include "Api.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "Utilities.h"
#include <cstddef>

namespace Graphics
{
PassId PassesManager::addPass(const char* name_, CmdTypeEnum cmdType_, QueueFamilyEnum queueFamily_, VkFence signalFence_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_, signalFence_);
    return {cmdType_, queueFamily_, this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].size() - 1}; 
}

Pass& PassesManager::getPass(PassId passId_)
{
    return this->passesPerCmdType[passId_.cmdType].passesPerQueue[passId_.queueFamily][passId_.id];
}


void PassesManager::deriveFrameVkSubmitInfos()
{
    PassesPack& rFramePassesPack = this->passesPerCmdType[FRAME];
    for(uint8_t i = 0; i < 3; i++) // per Queue 
    {
        // Frame 
        for(const auto& rPass : rFramePassesPack.passesPerQueue[i])
        {
            this->submissionBatchesPerQueue[i].emplace_back(false); 
            SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[i].back(); 
            for(auto& rTask : rPass.tasks)
            {
                // Add secondary buffer support later on.
                VkSubmitInfo submitInfo = {}; 
                submitInfo.pCommandBuffers = &getCommandBuffer(static_cast<QueueFamilyEnum>(i), FRAME, rTask.cmdId);
                submitInfo.commandBufferCount = 1;  
                submitInfo.pWaitSemaphores = rTask.waitSemaphores.data(); 
                submitInfo.waitSemaphoreCount = rTask.waitSemaphores.size(); 
                submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
                submitInfo.pSignalSemaphores = rTask.signalSemaphores.data(); 
                submitInfo.signalSemaphoreCount = rTask.signalSemaphores.size(); 
                rSubmissionBatch.submissions.emplace_back(submitInfo);
            }
        }
    }
} 

void PassesManager::deriveOneShotPass(QueueFamilyEnum queueFamily_, uint32_t id_)
{
    this->submissionBatchesPerQueue[queueFamily_].emplace_back(true); 
    SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[queueFamily_].back();
    for(const auto& rTask : this->passesPerCmdType[ONESHOT].passesPerQueue[queueFamily_][id_].tasks)
    {
        // Add secondary buffer support later on.
        VkSubmitInfo submitInfo = {}; 
        submitInfo.pCommandBuffers = &getCommandBuffer(queueFamily_, FRAME, rTask.cmdId);
        submitInfo.commandBufferCount = 1;  
        submitInfo.pWaitSemaphores = rTask.waitSemaphores.data(); 
        submitInfo.waitSemaphoreCount = rTask.waitSemaphores.size(); 
        submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
        submitInfo.pSignalSemaphores = rTask.signalSemaphores.data(); 
        submitInfo.signalSemaphoreCount = rTask.signalSemaphores.size(); 
        rSubmissionBatch.submissions.emplace_back(submitInfo);
    }
}; 
}