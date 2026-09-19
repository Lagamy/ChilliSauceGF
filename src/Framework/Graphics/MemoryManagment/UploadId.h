#pragma once 
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics {
    struct UploadId 
    {
        AllocatorTypeEnum allocatorType; 
        MemoryVisabilityEnum memoryVisability; 
        BufferTypeEnum uploadType; 
        PoolId id = UninitializedPoolId; // For static just use first field.  
    };
}
     