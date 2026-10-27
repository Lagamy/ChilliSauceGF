#pragma once
#include "Descriptor.h"
#include "Pool.h" 
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"

namespace Graphics
{
struct DescriptorsManager 
{
    Pool<DescriptorSetLayout> layouts; // Same layout can have multiple Descriptor Sets in it. 
    Pool<DescriptorSet> descriptorSet; // with Pool (since this framework has them 1 to 1. Since we use dynamic pools)
};
}

// Set guards for: 
// maxBoundDescriptorSets limits how many descriptor sets can be bound simultaneously through a pipeline layout. It does not limit how many VkDescriptorSet objects can exist or be allocated overall.
// VkDescriptorPoolCreateInfo::maxSets limits how many sets can be allocated from that particular pool.
// Descriptor-type limits, such as maxDescriptorSetUniformBuffers and maxDescriptorSetSampledImages, constrain descriptor usage according to their specific rules.