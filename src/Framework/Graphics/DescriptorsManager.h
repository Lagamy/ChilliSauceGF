#pragma once
#include "Descriptor.h"
#include "Pool.h" 
#include "PoolMap.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include <vector>

namespace Graphics
{
struct BoundDescriptor
{
    PoolId setId;
    PoolId descriptorId; // blueprint id, as in DescriptorSet::setDescriptorMemberValue
};

struct DescriptorsManager 
{
    PoolMap<DescriptorSetLayout> layouts = {"Descriptor Set Layouts"}; // Same layout can have multiple Descriptor Sets in it. 
    Pool<DescriptorSet> descriptorSet; // with Pool (since this framework has them 1 to 1. Since we use dynamic pools)
    std::vector<BoundDescriptor> boundDescriptors; // descriptors read by a draw recorded this frame, see lockBoundDescriptors

    PoolId addDescriptorSetLayout(const char* name_, DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_);
    PoolId addDescriptorBlueprint(const char* layoutName_, const char* blueprintName_);
    PoolId addMemberToDescriptorBlueprint(PoolId layoutId_, PoolId blueprintId_, const char* name_, DataTypeEnum dataType_);
    PoolId addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_);
    void removeDescriptorBlueprint(PoolId layoutId_, PoolId blueprintId_); // throws if the layout has derived sets
    void removeDescriptorSet(PoolId setId_); // waits for the frames that bind it, so don't call it while recording
    void removeDescriptorSetLayout(PoolId layoutId_); // throws if it has derived sets or a PipelineLayout uses it

    void allocateAllDescriptorsMemory(); // sets that weren't allocated yet. Before StaticAllocator::allocateAndUpload
    void createAll(); // layouts and sets that weren't created yet. After StaticAllocator::allocateAndUpload, before pipelines
    void syncToFrame(uint32_t frame_); // after PassesGraph::waitForFrame(frame_), before this frame's submit
    void queueLockOnSubmit(PoolId setId_, PoolId descriptorId_); // called by drawMeshIndexed for the descriptors its shaders declared
    void lockBoundDescriptors(); // right after the frame's submit
    void destroy();
};
}

// Set guards for: 
// maxBoundDescriptorSets limits how many descriptor sets can be bound simultaneously through a pipeline layout. It does not limit how many VkDescriptorSet objects can exist or be allocated overall.
// VkDescriptorPoolCreateInfo::maxSets limits how many sets can be allocated from that particular pool.
// Descriptor-type limits, such as maxDescriptorSetUniformBuffers and maxDescriptorSetSampledImages, constrain descriptor usage according to their specific rules.
