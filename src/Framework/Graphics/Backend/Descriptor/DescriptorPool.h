#pragma once 
#include "Utilities.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
struct DescriptorPool 
{
    VkDescriptorPool vkHandle; 
    std::vector<VkDescriptorPoolSize> poolSizePerType;
    std::array<uint32_t, 4> initializedDescriptorTypes = {UninitializedId, UninitializedId, UninitializedId, UninitializedId};

    void setDescriptorCountPerType(DescriptorTypeEnum descriptorType_, uint32_t count_); 
    void create(uint32_t maxSets_);
    void destroy();
};
}