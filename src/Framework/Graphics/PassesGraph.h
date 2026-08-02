#pragma once
#include "PassId.h"
#include "Pool.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "Task.h"
#include "Utilities.h"

namespace Graphics
{

struct PassesGraph 
{
    std::array<PassesPack, 2> passesPerCmdType; 
	std::vector<SubmissionBatch> submissionBatches;
    std::vector<PassId> passesOrder;
    
    bool orderDirty = false; 
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc singalFenceFunc_);

    Pass& getPass(PassId passId_);
    uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_);
    void addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_); 
    void addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_);
    // uint32_t addWaitFence();
    // uint32_t addFenceToReset();
    
    void enablePass(PassId passId_); 
    void disableFramePass(PassId passId_); // Since oneshot - self disables

    void compileIfDirty();
    void resolveDynamicSync(SubmissionBatch& rSubmissionBatch_, SubmissionMetadata& rMetadata); 

    void resolveSync_SubmitToGPU();     
};
}