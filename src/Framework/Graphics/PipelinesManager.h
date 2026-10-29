#pragma once 
#include "PipelineLayout.h"
#include "PoolMap.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

namespace Graphics
{
struct PipelinesManager
{
    Pool<PipelineLayout> pipelineLayouts; 
	PoolMap<GraphicsPipeline> graphicsPipelines = ("Graphics Pipelines");
    PoolMap<ComputePipeline> computePipelines = ("Compute Pipelines");

    PoolId addPipelineLayout();
    void addDescriptorSetLayoutToPipelineLayout(PoolId pipelineLayoutId_, PoolId descriptorSetLayoutId_);
    PoolId addGraphicsPipeline(const char* name_, PoolId pipelineLayoutId_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_);

    void createAllPipelines(); 
    void destroyAllPipelines();
}; 
}