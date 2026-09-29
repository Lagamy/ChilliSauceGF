#pragma once 
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics {
    struct MemoryEntryId 
    {
        AllocatorTypeEnum allocatorType;
        PoolId id = UninitializedPoolId; // For static just use first field.  
    };
}
     