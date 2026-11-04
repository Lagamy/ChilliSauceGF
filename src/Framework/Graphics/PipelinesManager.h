#pragma once 
#include "PipelineLayout.h"
#include "Pool.h"
#include "PoolMap.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

namespace Graphics
{
struct PipelinesManager
{
    Pool<PipelineLayout> pipelineLayouts = ("Pipeline Layouts");
	PoolMap<GraphicsPipeline> graphicsPipelines = ("Graphics Pipelines");
    PoolMap<ComputePipeline> computePipelines = ("Compute Pipelines");

    VkDescriptorSetLayout emptySetLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout getEmptySetLayout();
    PoolId getOrAddPipelineLayout(PoolId vertexShaderId_, PoolId fragmentShaderId_);
    PoolId addGraphicsPipeline(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_);

    void createAllPipelines(); 
    void destroyAllPipelines();
}; 
}