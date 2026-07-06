// Describes how rendering should happen(Rules and look of ingridients).
#pragma once

#include "RenderPass.h"
#include "Shader.h"
#include "PipelineLayout.h"
#include "Fence.h"
#include "Utilities.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
  
namespace Graphics
{
struct GraphicsPipeline {
	VkPipeline vkHandle = VK_NULL_HANDLE;
	PipelineLayout pipelineLayout;
 
	// In order to have a cached info for pipeline recreation 
	VkPrimitiveTopology primitiveType; // What Primitive do verticies represent(Points, lines, triangle, etc) 
	VkPolygonMode polygonMode; // How to Rasterize Primitives. How primitives are filled during rasterization(Select line for wireframe)
	PoolId vertexShaderId;
	PoolId fragmentShaderId;

	PoolId notInUseFenceId; // So we don't rebuild Pipeline, Descriptors and the sets when they are used by the GPU  
	
	// void createPipelineLayout(VkPushConstantRange* pushConstRangeList_, uint32_t pushConstRangeCount_, std::string context);
	
	//void create(const std::string& vertexShaderPath_, const std::string& fragmentShaderPath_, VkPrimitiveTopology _primitiveToUse, VkPolygonMode _howToRasterizePrimirives, PipelineLayout& _pipelineLayout, RenderPass& _renderPass, uint32_t _subpassIndex);
	void create(RenderPass& rRenderpass_, uint32_t subpassId_);
	void destroy();
	
	GraphicsPipeline(PoolId vertexShaderId_, PoolId fragmentShaderId_, VkPrimitiveTopology primitiveType_, VkPolygonMode polygonMode_) : vertexShaderId(vertexShaderId_), fragmentShaderId(fragmentShaderId_), primitiveType(primitiveType_), polygonMode(polygonMode_) {};
	GraphicsPipeline() = default; 
	VkPipeline get() const;
};
}
