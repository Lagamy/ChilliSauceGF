#pragma once
#include "PassId.h"
#include "PoolMap.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "CmdBuffers.h"
#include "Task.h"
#include "Utilities.h"

namespace Graphics
{

struct PassesGraph 
{
    std::array<PassesPack, 2> passesPerCmdType; 
	std::vector<SubmissionBatch> submissionBatches;
    std::vector<PassId> passesOrder;
    uint32_t enabledPassesCount = 0; 
    std::vector<PassId> heldPasses; // ONESHOT passes enablePass was called for, waiting for the descriptors their command lists use, see releaseHeldPasses
    
    bool orderDirty = false; 
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc singalFenceFunc_);

    Pass& getPass(PassId passId_);
    uint32_t addTaskToPass(PassId passId_, const char* name_, PoolId commandListId_); // the task is recorded from that CommandList
    void setTaskCommandList(PassId passId_, uint32_t taskId_, PoolId commandListId_); // Takes effect at the next recording, not while recording
    void registerTaskCmdBuffers(PassId passId_, uint32_t taskId_); // adds the task's command buffers to the pools (not allocated yet)
    void record(const CmdRecorder& rRecorder_, VkCommandBuffer& cmdBuffer_);
    void addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_); 
    void addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_);
    // uint32_t addWaitFence();
    // uint32_t addFenceToReset();
    
    void enablePass(PassId passId_); // a ONESHOT pass whose command lists use descriptors that are not loaded and settled yet is held back (heldPasses) instead
    void releaseHeldPasses(); // enablePass again for every held pass, the ones that are ready get enabled. Renderer::draw calls it after DescriptorsManager::updateLoading
    bool areDescriptorsReady(PassId passId_); // marks the descriptors the command lists of the pass's tasks use as used, true when all are loaded and settled. Tasks recorded from a function can't be inspected
    void disableFramePass(PassId passId_); // Since oneshot - self disables

    void waitForFrame(uint32_t frameIndex_);
    void compileIfDirty();
    void resolveDynamicSync(SubmissionBatch& rSubmissionBatch_, SubmissionMetadata& rMetadata); 
    void resolveSync_SubmitToGPU();     

    // void resolveDynamicSync(SubmissionBatch& rSubmissionBatch_); 
    // void resolveSync_SubmitToGPU();     
};
}