#pragma once
#include "Utilities.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics 
{
    namespace Parameters 
    {
        inline std::vector<const char*> additionalDeviceExtensions; // If you choose Ray tracing to be enabled -> this would change
        inline uint32_t dynamicMemChunkSize = 256 * storageUnitToBytes[MB]; 
    };
};
