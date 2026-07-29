#pragma once 
#include "Utilities.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics
{
struct SubmissionSync 
{
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkSemaphore> signalSemaphores;
}; 
}