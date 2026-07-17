#pragma once 
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics 
{
struct SubmissionBatch
{
    bool oneShot; 
    std::vector<VkSubmitInfo> submissions; 

    SubmissionBatch(bool oneShot_) : oneShot(oneShot_) {};
};
}