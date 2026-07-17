#pragma once
#include "PassId.h"
#include "PassesPack.h"
#include "Utilities.h"

namespace Graphics
{
struct PassesManager 
{
    std::array<PassesPack, 2> passesPerCmdType; 
	std::array<std::vector<SubmissionBatch>, 3> submissionBatchesPerQueue;

    PassId addPass(const char* name_, CmdTypeEnum cmdType_, QueueFamilyEnum queueFamily_, VkFence signalFence_);
    Pass& getPass(PassId passId_);
    
    void deriveFrameVkSubmitInfos(); 
    void deriveOneShotPass(QueueFamilyEnum queueFamily_, uint32_t id_);
};
}