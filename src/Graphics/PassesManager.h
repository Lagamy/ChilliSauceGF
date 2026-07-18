#pragma once
#include "PassId.h"
#include "Pool.h"
#include "PassesPack.h"
#include "Utilities.h"

namespace Graphics
{
struct PassesManager 
{
    std::array<PassesPack, 2> passesPerCmdType; 
	std::array<Pool<SubmissionBatch>, 3> submissionBatchesPerQueue; // Derives 1 time from passes, after all of them were declared 

    PassId addPass(const char* name_, CmdTypeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    Pass& getPass(PassId passId_);
    
    //void deriveFrameVkSubmitInfos(); 
    //void deriveOneShotPass(QueueFamilyEnum queueFamily_, size_t id_);
    void enablePass(PassId passId_); 
    void disableFramePass(PassId passId_); // Since oneshot - self disables
};
}