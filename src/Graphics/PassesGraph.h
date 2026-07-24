#pragma once
#include "PassId.h"
#include "Pool.h"
#include "PassesPack.h"
#include "Utilities.h"

namespace Graphics
{
struct PassesGraph 
{
    std::array<PassesPack, 2> passesPerCmdType; 
	std::array<Pool<SubmissionBatch>, 3> submissionBatchesPerQueue = { ("Graphics Submission Batches"), ("Transfer Submission Batches"), ("Compute Submission Batches")}; // Derives 1 time from passes, after all of them were declared 
    std::vector<PassId> passesOrder;
    bool orderDirty = false; 

    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_);

    Pass& getPass(PassId passId_);
    uint32_t addTaskToPass(PassId passId_, const char* name_, CmdBufferFunc cmdBufferFunc_);
    // uint32_t addWaitFence(); 
    // uint32_t addFenceToReset(); 
    
    void enablePass(PassId passId_); 
    void disableFramePass(PassId passId_); // Since oneshot - self disables
    
    void compileIfDirty();
    void submitToGPU();     
};
}