#include "PipelinesManager.h"
#include "Api.h"

namespace Graphics
{
PoolId PipelinesManager::addPipelineLayout()
{
    return this->pipelineLayouts.add();
}

/* The DescriptorSetLayout keeps the ids of the pipeline layouts using it, so it can rebuild them when its blueprints change. */
void PipelinesManager::addDescriptorSetLayoutToPipelineLayout(PoolId pipelineLayoutId_, PoolId descriptorSetLayoutId_)
{
    getDescriptorsManager().layouts[descriptorSetLayoutId_].usedByPipelineLayoutIds.add(pipelineLayoutId_);
    this->pipelineLayouts[pipelineLayoutId_].addDescriptorSetLayout(descriptorSetLayoutId_);
}

PoolId PipelinesManager::addGraphicsPipeline(const char* name_, PoolId pipelineLayoutId_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_)
{
    PoolId id = this->graphicsPipelines.add(name_, pipelineLayoutId_, vertexShaderId_, fragmentShaderId_, verticeLayoutId_, primitiveType_, polygonMode_, rRenderpass_, subpassId_);
    this->pipelineLayouts[pipelineLayoutId_].usedByGraphicsPipelineIds.emplace_back(id);
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
}
}
