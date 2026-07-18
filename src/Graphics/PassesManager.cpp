#include "PassesManager.h"
#include "Api.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "Utilities.h"
#include <cstddef>
#include <vector>

namespace Graphics
{
PassId PassesManager::addPass(const char* name_, CmdTypeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_)
{
    this->passesPerCmdType[cmdType_].passesPerQueue[queueFamily_].emplace_back(name_, signalFenceId_);
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
            this->submissionBatchesPerQueue[i].emplace_back(false, rPass.signalFenceId); 
            SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[i].back(); 
            for(auto& rTask : rPass.tasks)
            {
                std::vector<VkSemaphore> waitSemaphores;
                waitSemaphores.resize(rTask.waitSemaphoresIds.size());
                for(int j = 0; j < waitSemaphores.size(); j++)
                {
                    waitSemaphores[j] = getSemaphore(rTask.waitSemaphoresIds[j]); 
                }
                

                std::vector<VkSemaphore> signalSemaphores;
                for(int j = 0; j < signalSemaphores.size(); j++)
                {
                    signalSemaphores[j] = getSemaphore(rTask.signalSemaphoresIds[j]); 
                }
                signalSemaphores.resize(rTask.signalSemaphoresIds.size()); 
 
                // Add secondary buffer support later on.
                VkSubmitInfo submitInfo = {}; 
                submitInfo.pCommandBuffers = &getCommandBuffer(static_cast<QueueFamilyEnum>(i), FRAME, rTask.cmdId);
                submitInfo.commandBufferCount = 1;  
                submitInfo.pWaitSemaphores = waitSemaphores.data(); 
                submitInfo.waitSemaphoreCount = waitSemaphores.size(); 
                submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
                submitInfo.pSignalSemaphores = signalSemaphores.data(); 
                submitInfo.signalSemaphoreCount = signalSemaphores.size(); 
                rSubmissionBatch.submissions.emplace_back(submitInfo);
            }
        }
    }
} 

void PassesManager::deriveOneShotPass(QueueFamilyEnum queueFamily_, size_t id_)
{
    this->submissionBatchesPerQueue[queueFamily_].emplace_back(true, this->passesPerCmdType[ONESHOT].passesPerQueue[queueFamily_][id_].signalFenceId); 
    SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[queueFamily_].back();
    for(const auto& rTask : this->passesPerCmdType[ONESHOT].passesPerQueue[queueFamily_][id_].tasks)
    {
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
        submitInfo.pCommandBuffers = &getCommandBuffer(queueFamily_, FRAME, rTask.cmdId);
        submitInfo.commandBufferCount = 1;  
        submitInfo.pWaitSemaphores = waitSemaphores.data(); 
        submitInfo.waitSemaphoreCount = waitSemaphores.size(); 
        submitInfo.pWaitDstStageMask = rTask.waitStages.data(); 
        submitInfo.pSignalSemaphores = signalSemaphores.data(); 
        submitInfo.signalSemaphoreCount = signalSemaphores.size(); 
        rSubmissionBatch.submissions.emplace_back(submitInfo);
    }
}; 

void PassesManager::enablePass(PassId passId_)
{
    Pass& rPass = this->getPass(passId_);
    PoolId submissionId = this->submissionBatchesPerQueue[passId_.queueFamily].add(rPass.name.c_str(), static_cast<bool>(passId_.cmdType), rPass.signalFenceId); 
    rPass.submissionId = submissionId; 
    SubmissionBatch& rSubmissionBatch = this->submissionBatchesPerQueue[passId_.queueFamily].get(submissionId);
    for(const auto& rTask : rPass.tasks)
    {
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
    }
} 

void PassesManager::disableFramePass(PassId passId_)
{
    Pass& rPass = getPass(passId_); 
    this->submissionBatchesPerQueue[passId_.queueFamily].remove(rPass.submissionId); 
    rPass.submissionId = UninitializedPoolId; 
}
}