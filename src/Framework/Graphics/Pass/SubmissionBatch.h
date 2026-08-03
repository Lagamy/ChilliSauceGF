#pragma once 
#include "PoolId.h"
#include "SubmissionMetadata.h"
#include "Utilities.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics 
{

// struct SubmissionId 
// {
//     CmdLifetimeEnum lifetime; 
//     uint32_t id; 
// };

struct SubmissionBatch
{
    std::string name; 
    CmdLifetimeEnum lifetime;
    QueueFamilyEnum queueFamily; 
    std::vector<SubmissionMetadata> perSubmissionMetadata; 
    std::vector<VkSubmitInfo> submitInfos; 
    
    std::vector<uint32_t> cmdBuffersToDisable; 
    SyncRetrivalFunc dynamicSignalFenceFunc; 
    PoolId signalFenceId = UninitializedPoolId; 

    std::vector<uint32_t> submissionsWithDynamicSync; 
    bool dynamicSignalFence = false; 

    SubmissionBatch(std::string& name_, CmdLifetimeEnum lifetime_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    SubmissionBatch(std::string& name_, CmdLifetimeEnum lifetime_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc signalRetrivalFunc_);
    
    
    ~SubmissionBatch() {}; 
};
}