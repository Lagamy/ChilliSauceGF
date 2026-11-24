#include "PipelinesManager.h"
#include "Api.h"
#include <algorithm>

namespace Graphics
{
/*
    The set layouts of the pipeline layout come from the descriptors both shaders declared: the layout declared for set N goes to position N (the position is the set index in Vulkan).
    Both shaders have to agree on a set they both use. An index nobody declared (a hole below the highest one) stays UninitializedPoolId; PipelineLayout::create fills it with the empty set layout.
    A layout with the same ordered list already exists is reused: pipelines with the same set layouts share one VkPipelineLayout, and sets bound for one stay valid for the other.
    The push constant uses (sorted by stages, so the order they were attached in doesn't matter) are part of the key too: the ranges of the VkPipelineLayout come from them.
    A new layout registers itself on its set layouts, so they can rebuild it when their descriptor layouts change.
*/
PoolId PipelinesManager::getOrAddPipelineLayout(PoolId vertexShaderId_, PoolId fragmentShaderId_, const std::vector<PushConstantUse>& pushConstantUses_)
{
    std::vector<PoolId> setLayoutIds;
    for(PoolId shaderId : {vertexShaderId_, fragmentShaderId_})
    {
        for(const ShaderDescriptorUse& rUse : getShader(shaderId).descriptors)
        {
            if(rUse.setIndex >= setLayoutIds.size())
            {
                setLayoutIds.resize(rUse.setIndex + 1, UninitializedPoolId);
            }
            if(setLayoutIds[rUse.setIndex] != UninitializedPoolId && setLayoutIds[rUse.setIndex] != rUse.setLayoutId)
            {
                throw std::runtime_error(std::format("Pipelines Manager: the shaders of a pipeline declare different set layouts for set {}.", rUse.setIndex));
            }
            setLayoutIds[rUse.setIndex] = rUse.setLayoutId;
        }
    }

    for(uint32_t i = 0; i < this->pipelineLayouts.allocatedSize(); i++)
    {
        if(!this->pipelineLayouts.alive[i])
        {
            continue;
        }
        const PipelineLayout& rExisting = this->pipelineLayouts.getInternal(i);
        bool sameUses = rExisting.pushConstantUses.size() == pushConstantUses_.size();
        for(uint32_t useId = 0; sameUses && useId < pushConstantUses_.size(); useId++)
        {
            sameUses = rExisting.pushConstantUses[useId].layoutId == pushConstantUses_[useId].layoutId && rExisting.pushConstantUses[useId].stages == pushConstantUses_[useId].stages;
        }
        if(sameUses && rExisting.descriptorSetLayoutIds == setLayoutIds)
        {
            return PoolId(i, this->pipelineLayouts.generation[i]);
        }
    }

    PoolId pipelineLayoutId = this->pipelineLayouts.add();
    this->pipelineLayouts[pipelineLayoutId].descriptorSetLayoutIds = setLayoutIds;
    this->pipelineLayouts[pipelineLayoutId].pushConstantUses = pushConstantUses_;
    for(const PoolId& rSetLayoutId : setLayoutIds)
    {
        if(rSetLayoutId == UninitializedPoolId)
        {
            continue;
        }
        getDescriptorsManager().layouts[rSetLayoutId].usedByPipelineLayoutIds.add(pipelineLayoutId);
    }
    return pipelineLayoutId;
}

PoolId PipelinesManager::addGraphicsPipeline(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_)
{
    PoolId pipelineLayoutId = this->getOrAddPipelineLayout(vertexShaderId_, fragmentShaderId_, {});
    PoolId id = this->graphicsPipelines.add(name_, pipelineLayoutId, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
    this->pipelineLayouts[pipelineLayoutId].usedByGraphicsPipelineIds.emplace_back(id);
    return id;
}

/*
    Attaches a push constant layout to a pipeline for the given stages, before the pipelines are created (createAllPipelines). The pipeline layout was derived from the shaders when the pipeline was added, so it is resolved again with the new use: pipelines with the same set layouts and the same push constant uses share one VkPipelineLayout.
    A pipeline layout has one push constant range per shader stage, so the stages of two layouts of a pipeline can't overlap. Where the layouts sit in the push constant range is decided when the pipeline layout is created (PushConstantStorage), read it with getPushConstantOffset.
    The layout the pipeline left is removed when nothing else uses it and it was never created, it would only be an unused VkPipelineLayout.
*/
void PipelinesManager::addPushConstantLayoutToPipeline(PoolId graphicsPipelineId_, PoolId pushConstantLayoutId_, VkShaderStageFlags stages_)
{
    GraphicsPipeline& rPipeline = this->graphicsPipelines[graphicsPipelineId_];
    PushConstantLayout& rPushConstantLayout = this->pushConstantLayouts[pushConstantLayoutId_];
    if(rPipeline.vkHandle != VK_NULL_HANDLE || this->pipelineLayouts[rPipeline.pipelineLayoutId].vkHandle != VK_NULL_HANDLE)
    {
        throw std::runtime_error(std::format("Pipelines Manager: push constant layout {} can't be attached, the pipeline {} was already created.", rPushConstantLayout.name, this->graphicsPipelines.getName(graphicsPipelineId_)));
    }
    if(stages_ == 0 || (stages_ & ~VK_SHADER_STAGE_ALL_GRAPHICS) != 0)
    {
        throw std::runtime_error(std::format("Pipelines Manager: push constant layout {} needs graphics shader stages.", rPushConstantLayout.name));
    }
    for(const PushConstantUse& rUse : rPipeline.pushConstantUses)
    {
        if(rUse.layoutId == pushConstantLayoutId_)
        {
            throw std::runtime_error(std::format("Pipelines Manager: push constant layout {} is already attached to the pipeline {}.", rPushConstantLayout.name, this->graphicsPipelines.getName(graphicsPipelineId_)));
        }
        if((rUse.stages & stages_) != 0)
        {
            throw std::runtime_error(std::format("Pipelines Manager: push constant layout {} shares a shader stage with {} on the pipeline {}, a pipeline layout has one push constant range per stage.", rPushConstantLayout.name, this->pushConstantLayouts[rUse.layoutId].name, this->graphicsPipelines.getName(graphicsPipelineId_)));
        }
    }

    PushConstantUse use;
    use.layoutId = pushConstantLayoutId_;
    use.stages = stages_;
    rPipeline.pushConstantUses.push_back(use);
    std::sort(rPipeline.pushConstantUses.begin(), rPipeline.pushConstantUses.end(), [](const PushConstantUse& a_, const PushConstantUse& b_) { return a_.stages < b_.stages; });

    PoolId oldPipelineLayoutId = rPipeline.pipelineLayoutId;
    std::erase(this->pipelineLayouts[oldPipelineLayoutId].usedByGraphicsPipelineIds, graphicsPipelineId_);
    PoolId newPipelineLayoutId = this->getOrAddPipelineLayout(rPipeline.vertexShaderId, rPipeline.fragmentShaderId, rPipeline.pushConstantUses);
    rPipeline.pipelineLayoutId = newPipelineLayoutId;
    this->pipelineLayouts[newPipelineLayoutId].usedByGraphicsPipelineIds.emplace_back(graphicsPipelineId_);

    /* Taken after getOrAddPipelineLayout, adding a layout can move the pool's storage. */
    PipelineLayout& rOldLayout = this->pipelineLayouts[oldPipelineLayoutId];
    if(rOldLayout.usedByGraphicsPipelineIds.empty() && rOldLayout.vkHandle == VK_NULL_HANDLE)
    {
        for(const PoolId& rSetLayoutId : rOldLayout.descriptorSetLayoutIds)
        {
            if(rSetLayoutId == UninitializedPoolId)
            {
                continue;
            }
            Pool<PoolId>& rUsedBy = getDescriptorsManager().layouts[rSetLayoutId].usedByPipelineLayoutIds;
            for(uint32_t i = 0; i < rUsedBy.allocatedSize(); i++)
            {
                if(rUsedBy.alive[i] && rUsedBy.getInternal(i) == oldPipelineLayoutId)
                {
                    rUsedBy.removeInternal(i);
                }
            }
        }
        this->pipelineLayouts.remove(oldPipelineLayoutId);
    }
}

/* The staged copy is taken here: the live layout is not touched until the commit. */
PoolId PipelinesManager::beginPushConstantLayoutUpdate(PoolId pushConstantLayoutId_)
{
    PushConstantLayout& rLayout = this->pushConstantLayouts[pushConstantLayoutId_];
    if(rLayout.updateOpen)
    {
        throw std::runtime_error(std::format("Push Constant Layout {}: An update of it is already open, commit it first.", rLayout.name));
    }
    rLayout.updateOpen = true;
    return this->pushConstantLayoutUpdates.add(pushConstantLayoutId_, rLayout);
}

PoolId PipelinesManager::addMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    return this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.addMemberBlueprint(name_, orderPosition_, dataType_, repeatCountPerDimension_);
}

PoolId PipelinesManager::addMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    return this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.addMemberBlueprint(name_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
}

PoolId PipelinesManager::addStructMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_)
{
    return this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.addStructMemberBlueprint(name_, orderPosition_, repeatCountPerDimension_, headroom_);
}

PoolId PipelinesManager::addVariableToStructInPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    return this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.addVariable(name_, structMemberId_, orderPosition_, dataType_, repeatCountPerDimension_);
}

PoolId PipelinesManager::addVariableToStructInPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    return this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.addVariable(name_, structMemberId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
}

void PipelinesManager::removeMemberFromPushConstantLayout(PoolId pushConstantLayoutUpdateId_, PoolId memberId_)
{
    this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.removeMemberBlueprint(memberId_);
}

void PipelinesManager::removeVariableFromStructInPushConstantLayout(PoolId pushConstantLayoutUpdateId_, PoolId variableId_)
{
    this->pushConstantLayoutUpdates[pushConstantLayoutUpdateId_].staged.removeVariableFromStruct(variableId_);
}

/*
    The staged members replace the live ones. The pipeline layouts that use the layout and exist are created again (PipelineLayout::recreate: PushConstantStorage places the blocks again, so the offsets of the layouts after it can move, and the pipelines are created again with it), after the device is idle:
    a pipeline layout and the pipelines created with it can't be destroyed while a submitted command buffer still uses them (the valid usage of vkDestroyPipeline: every submitted command that refers to it has to have completed).
    Before anything is destroyed the new placement is tried on a copy of every affected layout's uses, so a block that no longer fits in maxPushConstantsSize throws with the old layout still in place and the update still open. Pipeline layouts that were not created yet are placed when they are.
    Commands that cached bytes (a value passed by value) were laid out for the old block and have to be added again, a command whose variable was removed throws when it is recorded.
*/
void PipelinesManager::commitPushConstantLayoutUpdate(PoolId updateId_)
{
    PushConstantLayoutUpdate& rUpdate = this->pushConstantLayoutUpdates[updateId_];
    PushConstantLayout& rLayout = this->pushConstantLayouts[rUpdate.layoutId];

    std::vector<uint32_t> createdPipelineLayoutSlots;
    for(uint32_t i = 0; i < this->pipelineLayouts.allocatedSize(); i++)
    {
        if(!this->pipelineLayouts.alive[i] || this->pipelineLayouts.getInternal(i).vkHandle == VK_NULL_HANDLE)
        {
            continue;
        }
        for(const PushConstantUse& rUse : this->pipelineLayouts.getInternal(i).pushConstantUses)
        {
            if(rUse.layoutId == rUpdate.layoutId)
            {
                createdPipelineLayoutSlots.push_back(i);
                break;
            }
        }
    }

    std::swap(rLayout.members, rUpdate.staged.members);
    std::swap(rLayout.variables, rUpdate.staged.variables);
    try
    {
        for(const uint32_t& rSlot : createdPipelineLayoutSlots)
        {
            std::vector<PushConstantUse> uses = this->pipelineLayouts.getInternal(rSlot).pushConstantUses;
            getMemoryManager().pushConstantStorage.place(uses);
        }
    }
    catch(...)
    {
        std::swap(rLayout.members, rUpdate.staged.members);
        std::swap(rLayout.variables, rUpdate.staged.variables);
        throw;
    }

    if(!createdPipelineLayoutSlots.empty())
    {
        vkDeviceWaitIdle(getMainDevice().logicalDevice);
        for(const uint32_t& rSlot : createdPipelineLayoutSlots)
        {
            this->pipelineLayouts.getInternal(rSlot).recreate();
        }
    }
    rLayout.updateOpen = false;
    this->pushConstantLayoutUpdates.remove(updateId_);
}

/* The layout attached to the pipeline that was added with that name (the layout names are unique, a layout can be attached once per pipeline). */
PoolId PipelinesManager::getPushConstantLayoutId(const char* name_, PoolId graphicsPipelineId_)
{
    for(const PushConstantUse& rUse : this->graphicsPipelines[graphicsPipelineId_].pushConstantUses)
    {
        if(this->pushConstantLayouts.getName(rUse.layoutId) == name_)
        {
            return rUse.layoutId;
        }
    }
    throw std::runtime_error(std::format("Pipelines Manager: the pipeline {} has no push constant layout named {}.", this->graphicsPipelines.getName(graphicsPipelineId_), name_));
}

/* The offset of the block of a push constant layout in the push constant range of the pipeline, i.e. what the shader's block declares as layout(offset = N). Known once the pipeline layout was created (createAllPipelines). */
uint32_t PipelinesManager::getPushConstantOffset(PoolId graphicsPipelineId_, PoolId pushConstantLayoutId_)
{
    const PipelineLayout& rLayout = this->pipelineLayouts[this->graphicsPipelines[graphicsPipelineId_].pipelineLayoutId];
    if(rLayout.vkHandle == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Pipelines Manager: the push constant offsets are placed when the pipeline layout is created (createAllPipelines).");
    }
    return rLayout.getPushConstantUse(pushConstantLayoutId_).offset;
}

/* Layouts first: a pipeline is created with the layout's handle. Already created ones are skipped, so it can run again after the scene defined new pipelines. */
void PipelinesManager::createAllPipelines()
{
    for(uint32_t i = 0; i < this->pipelineLayouts.allocatedSize(); i++)
    {
        if(this->pipelineLayouts.alive[i] && this->pipelineLayouts.getInternal(i).vkHandle == VK_NULL_HANDLE)
        {
            this->pipelineLayouts.getInternal(i).create();
        }
    }

    for(uint32_t i = 0; i < this->graphicsPipelines.allocatedSize(); i++)
    {
        if(this->graphicsPipelines.alive[i] && this->graphicsPipelines.getInternal(i).vkHandle == VK_NULL_HANDLE)
        {
            this->graphicsPipelines.getInternal(i).create();
        }
    }
} 

void PipelinesManager::destroyAllPipelines()
{
    this->graphicsPipelines.clear(); 
    this->computePipelines.clear();
    this->pipelineLayouts.clear();
    this->pushConstantLayouts.clear();
    this->pushConstantLayoutUpdates.clear();
    if(this->emptySetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(getMainDevice().logicalDevice, this->emptySetLayout, nullptr);
        this->emptySetLayout = VK_NULL_HANDLE;
    }
}

/*
    A pipeline layout can't have a null set layout in pSetLayouts without the graphics pipeline library feature, so a set nobody declared gets a real layout with zero bindings (valid: bindingCount may be 0).
    Nothing is ever bound at that index. Created on first use, shared by every pipeline layout.
*/
VkDescriptorSetLayout PipelinesManager::getEmptySetLayout()
{
    if(this->emptySetLayout == VK_NULL_HANDLE)
    {
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = 0;
        layoutCreateInfo.pBindings = nullptr;

        VkResult result = vkCreateDescriptorSetLayout(getMainDevice().logicalDevice, &layoutCreateInfo, nullptr, &this->emptySetLayout);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Pipelines Manager: failed to create the empty descriptor set layout!");
        }
    }
    return this->emptySetLayout;
}
}
