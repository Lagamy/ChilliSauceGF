#pragma once 
#include "Pool.h"
#include <vulkan/vulkan.h> 

namespace Graphics
{
struct SubmitionBatch 
{
    Pool<VkSubmitInfo> submitionEntries; 
};
}