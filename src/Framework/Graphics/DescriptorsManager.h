#pragma once
#include "Descriptor.h"
#include "Pool.h" 
#include "PoolMap.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"

namespace Graphics
{
struct DescriptorsManager 
{
    PoolMap<DescriptorSetLayout> layouts = {"Descriptor Set Layouts"}; // Same layout can have multiple Descriptor Sets in it. 
    Pool<DescriptorSet> descriptorSet; // with Pool (since this framework has them 1 to 1. Since we use dynamic pools)

    PoolId addDescriptorSetLayout(const char* name_, DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_);
    PoolId addDescriptorBlueprint(const char* layoutName_, const char* blueprintName_);
    PoolId addMemberToDescriptorBlueprint(PoolId layoutId_, PoolId blueprintId_, const char* name_, DataTypeEnum dataType_);
    PoolId addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_);

    void allocateAllDescriptorsMemory(); // sets that weren't allocated yet. Before StaticAllocator::allocateAndUpload
    void createAll(); // layouts and sets that weren't created yet. After StaticAllocator::allocateAndUpload, before pipelines
    void destroy();
};
}

// Set guards for: 
// maxBoundDescriptorSets limits how many descriptor sets can be bound simultaneously through a pipeline layout. It does not limit how many VkDescriptorSet objects can exist or be allocated overall.
// VkDescriptorPoolCreateInfo::maxSets limits how many sets can be allocated from that particular pool.
// Descriptor-type limits, such as maxDescriptorSetUniformBuffers and maxDescriptorSetSampledImages, constrain descriptor usage according to their specific rules.
