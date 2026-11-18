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
    PoolId descriptorId; // descriptor layout id, as in DescriptorSet::setDescriptorMemberValue
};

struct DescriptorsManager 
{
    PoolMap<DescriptorSetLayout> layouts = {"Descriptor Set Layouts"}; // Same layout can have multiple Descriptor Sets in it. 
    Pool<DescriptorSet> descriptorSet; // with Pool (since this framework has them 1 to 1. Since we use dynamic pools)
    std::vector<BoundDescriptor> boundDescriptors; // descriptors read by a draw recorded this frame, see lockBoundDescriptors

    PoolId addDescriptorSetLayout(const char* name_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_);
    PoolId getLayoutId(const char* layoutName_);
    PoolId addBufferDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_);
    PoolId addBufferDynamicDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_, uint32_t dynamicCount_);
    PoolId addImageDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_);
    PoolId addMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_); // Matrix
    PoolId addStructMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addVariableToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addVariableToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_); // Matrix
    PoolId addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_);
    void removeDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_); // throws if the layout has derived sets
    void removeDescriptorSet(PoolId setId_); // waits for the frames that bind it, so don't call it while recording
    void removeDescriptorSetLayout(PoolId layoutId_); // throws if it has derived sets or a PipelineLayout uses it

    void allocateAllDescriptorsMemory(); // sets that weren't allocated yet. Before StaticAllocator::allocateAndUpload
    void createAll(); // layouts and sets that weren't created yet. After StaticAllocator::allocateAndUpload, before pipelines
    void queueLockOnSubmit(PoolId setId_, PoolId descriptorId_); // called by drawMeshIndexed for the descriptors its shaders declared
    void lockBoundDescriptors(); // right after the frame's submit
    void destroy();
};
}

// Set guards for: 
// maxBoundDescriptorSets limits how many descriptor sets can be bound simultaneously through a pipeline layout. It does not limit how many VkDescriptorSet objects can exist or be allocated overall.
// VkDescriptorPoolCreateInfo::maxSets limits how many sets can be allocated from that particular pool (DescriptorSetLayout adds pools when one is full).
// Descriptor-type limits, such as maxDescriptorSetUniformBuffers and maxDescriptorSetSampledImages, constrain descriptor usage according to their specific rules.
