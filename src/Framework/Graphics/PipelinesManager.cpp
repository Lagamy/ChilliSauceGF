#include "PipelinesManager.h"
#include "Api.h"

namespace Graphics
{
/*
    The set layouts of the pipeline layout come from the descriptors both shaders declared: the layout declared for set N goes to position N (the position is the set index in Vulkan).
    Both shaders have to agree on a set they both use. An index nobody declared (a hole below the highest one) stays UninitializedPoolId; PipelineLayout::create fills it with the empty set layout.
    A layout with the same ordered list already exists is reused: pipelines with the same set layouts share one VkPipelineLayout, and sets bound for one stay valid for the other.
    A new layout registers itself on its set layouts, so they can rebuild it when their blueprints change.
*/
PoolId PipelinesManager::getOrAddPipelineLayout(PoolId vertexShaderId_, PoolId fragmentShaderId_)
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
        if(this->pipelineLayouts.alive[i] && this->pipelineLayouts.getInternal(i).descriptorSetLayoutIds == setLayoutIds)
        {
            return PoolId(i, this->pipelineLayouts.generation[i]);
        }
    }

    PoolId pipelineLayoutId = this->pipelineLayouts.add();
    this->pipelineLayouts[pipelineLayoutId].descriptorSetLayoutIds = setLayoutIds;
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
    PoolId pipelineLayoutId = this->getOrAddPipelineLayout(vertexShaderId_, fragmentShaderId_);
    PoolId id = this->graphicsPipelines.add(name_, pipelineLayoutId, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
    this->pipelineLayouts[pipelineLayoutId].usedByGraphicsPipelineIds.emplace_back(id);
    return id;
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
