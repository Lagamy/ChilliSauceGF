#pragma once 
#include "PoolId.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics 
{
struct SubmissionBatch
{
    bool oneShot; 
    std::vector<VkSubmitInfo> submissions;
    std::vector<uint32_t> cmdBuffersToDisable; 
    PoolId signalFenceId; 

    SubmissionBatch(bool oneShot_, PoolId signalFenceId_) : oneShot(oneShot_), signalFenceId(signalFenceId_) {};
};
}