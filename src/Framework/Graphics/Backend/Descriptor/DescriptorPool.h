#pragma once
#include "Utilities.h"
#include <array>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
struct DescriptorPool
{
    VkDescriptorPool vkHandle = VK_NULL_HANDLE;
    std::vector<VkDescriptorPoolSize> poolSizePerType;
    std::array<uint32_t, DescriptorTypesCount * 2> initializedDescriptorTypes = []{ std::array<uint32_t, DescriptorTypesCount * 2> a; a.fill(UninitializedId); return a; }();

    void setDescriptorCountPerType(DescriptorTypeEnum descriptorType_, bool dynamic_, uint32_t count_); // dynamic and plain descriptors of a type are different Vulkan types, so they are counted apart
    void create(uint32_t maxSets_);
    void destroy();
};
}
