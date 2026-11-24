#pragma once 
#include "PipelineLayout.h"
#include "PushConstantLayout.h"
#include "PushConstantLayoutUpdate.h"
#include "Pool.h"
#include "PoolMap.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

namespace Graphics
{
struct PipelinesManager
{
    Pool<PipelineLayout> pipelineLayouts = ("Pipeline Layouts");
    PoolMap<PushConstantLayout> pushConstantLayouts = ("Push Constant Layouts");
    Pool<PushConstantLayoutUpdate> pushConstantLayoutUpdates; // open updates, removed by their commit
	PoolMap<GraphicsPipeline> graphicsPipelines = ("Graphics Pipelines");
    PoolMap<ComputePipeline> computePipelines = ("Compute Pipelines");

    VkDescriptorSetLayout emptySetLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout getEmptySetLayout();
    PoolId getOrAddPipelineLayout(PoolId vertexShaderId_, PoolId fragmentShaderId_, const std::vector<PushConstantUse>& pushConstantUses_);
    void addPushConstantLayoutToPipeline(PoolId graphicsPipelineId_, PoolId pushConstantLayoutId_, VkShaderStageFlags stages_);
    PoolId getPushConstantLayoutId(const char* name_, PoolId graphicsPipelineId_);
    PoolId beginPushConstantLayoutUpdate(PoolId pushConstantLayoutId_);
    void commitPushConstantLayoutUpdate(PoolId updateId_);
    PoolId addMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addStructMemberToPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_);
    PoolId addVariableToStructInPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_);
    PoolId addVariableToStructInPushConstantLayout(const char* name_, PoolId pushConstantLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_);
    void removeMemberFromPushConstantLayout(PoolId pushConstantLayoutUpdateId_, PoolId memberId_);
    void removeVariableFromStructInPushConstantLayout(PoolId pushConstantLayoutUpdateId_, PoolId variableId_);
    uint32_t getPushConstantOffset(PoolId graphicsPipelineId_, PoolId pushConstantLayoutId_);
    PoolId addGraphicsPipeline(const char* name_, PoolId vertexShaderId_, PoolId fragmentShaderId_, PoolId verticeLayoutId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_, RenderPass& rRenderpass_, uint32_t subpassId_);

    void createAllPipelines(); 
    void destroyAllPipelines();
}; 
}