#pragma once
#include "Descriptor.h"
#include "Pool.h" 
#include "PoolMap.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DescriptorLayoutUpdate.h"
#include "SetLayoutUpdate.h"
#include <vector>

namespace Graphics
{
struct DescriptorsManager 
{
    PoolMap<DescriptorSetLayout> layouts = {"Descriptor Set Layouts"}; // Same layout can have multiple Descriptor Sets in it. 
    Pool<DescriptorLayoutUpdate> descriptorLayoutUpdates; // open updates, removed by their commit
    Pool<SetLayoutUpdate> setLayoutUpdates;
    Pool<DescriptorSet> descriptorSet; // with Pool (since this framework has them 1 to 1. Since we use dynamic pools)
    std::vector<BoundDescriptor> boundDescriptors; // descriptors read by a draw recorded this frame, see lockBoundDescriptors
    std::vector<BoundDescriptor> loadsToWaitFor; // descriptors of WAIT_UNTIL_LOADED sets loaded this frame, see waitForLoads

    PoolId addDescriptorSetLayout(const char* name_, uint32_t setIndex_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_);
    PoolId getLayoutId(const char* layoutName_);
    /*
        Runtime editing of descriptor layouts, see DescriptorLayoutUpdate and SetLayoutUpdate: begin... opens an update of a layout, the add / remove functions edit it (they take the update id, which knows its layout) and commit... applies it, consuming the update.
        Before anything of the layout exists (the define* of a scene) a commit only writes the changes into the layouts, with live objects it waits for the device and reallocates, copies and rewrites what the change touches. Not while recording.
    */
    PoolId beginSetLayoutUpdate(PoolId layoutId_);
    PoolId beginDescriptorLayoutUpdate(PoolId layoutId_, PoolId descriptorLayoutId_);
    PoolId addBufferDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t headroom_ = 0);
    PoolId addDynamicDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, uint32_t headroom_ = 0);
    PoolId addImageDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_);
    void removeDescriptorLayout(PoolId setLayoutUpdateId_, PoolId descriptorLayoutId_); // takes effect at the commit
    PoolId addMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_); // Matrix
    PoolId addStructMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_ = 0);
    PoolId addVariableToStructInDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addVariableToStructInDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_); // Matrix
    void removeMemberFromDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId memberId_); // a whole member and its variables
    void removeVariableFromStructInDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId variableId_); // one variable of a struct member
    void commitSetLayoutUpdate(PoolId updateId_);
    void commitDescriptorLayoutUpdate(PoolId updateId_);
    PoolId addDescriptorSet(const DescriptorSetCreateParams& rParams_);
    void removeDescriptorSet(PoolId setId_); // waits for the frames that bind it, so don't call it while recording
    void removeDescriptorSetLayout(PoolId layoutId_); // throws if it has derived sets or a PipelineLayout uses it

    void allocateAllDescriptorsMemory(); // sets that weren't allocated yet. Before StaticAllocator::allocateAndUpload
    void createAll(); // layouts and sets that weren't created yet. After StaticAllocator::allocateAndUpload, before pipelines
    void queueLockOnSubmit(PoolId setId_, PoolId descriptorId_); // called by drawMeshIndexed for the descriptors its shaders declared
    void lockBoundDescriptors(); // right after the frame's submit
    void destroy();

    /*
        On demand loading of descriptors (DescriptorSet::loadDescriptor), per descriptor, see DescriptorSetCreateParams::residency (ResidencyParams). updateLoading runs in Renderer::draw every frame (after the frame's fence wait, before the shadows are copied): it loads the descriptors a draw or pass requested and unloads the ones nobody used for their set's residency.absenceSecondsCount seconds.
        Anything it changes needs the submits that used the descriptor finished first (PassesGraph::waitForDescriptorUse). Not while recording.
        loadDescriptor / unloadDescriptor are the manual versions (Api.h).
    */
    void updateLoading();
    void loadDescriptor(PoolId setId_, PoolId descriptorId_);
    void unloadDescriptor(PoolId setId_, PoolId descriptorId_);
    MemoryEntryId getDescriptorMemoryEntry(PoolId setId_, PoolId descriptorId_);
    void saveDescriptorToDisk(PoolId setId_, PoolId descriptorId_); // calls the set's saveToDisk, throws if the set has none
    void changeDescriptorLayoutOnDisk(PoolId setId_, PoolId descriptorId_); // calls the set's changeLayoutOnDisk, throws if the set has none
    void waitForLoads(); // after the frame's submit: blocks until the uploads of the descriptors loaded this frame by a WAIT_UNTIL_LOADED set completed
    void waitForGpu(); // vkDeviceWaitIdle, for the structural changes (layout commits, removals). Loads and unloads wait with PassesGraph::waitForDescriptorUse instead
};
}

// Set guards for: 
// maxBoundDescriptorSets limits how many descriptor sets can be bound simultaneously through a pipeline layout. It does not limit how many VkDescriptorSet objects can exist or be allocated overall.
// VkDescriptorPoolCreateInfo::maxSets limits how many sets can be allocated from that particular pool (DescriptorSetLayout adds pools when one is full).
// Descriptor-type limits, such as maxDescriptorSetUniformBuffers and maxDescriptorSetSampledImages, constrain descriptor usage according to their specific rules.
