#pragma once 
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics {
    struct UploadId 
    {
        AllocatorTypeEnum allocatorType; 
        MemoryVisabilityEnum memoryVisability; 
        BufferTypeEnum uploadType; 
        VkDeviceSize id; 
    };
}
     