#pragma once 
#include "PoolId.h"
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

namespace Graphics 
{
struct TriangleVertexMemberIds 
{
	PoolId positionId; 
	PoolId colorId; 
}; 

namespace Triangle { 
	inline PoolId vertexShaderId; 
	inline PoolId fragmentShaderId; 
	inline PoolId vertexLayoutId; 
	inline TriangleVertexMemberIds vertexMemberIds; 

	inline PoolId meshId;
	inline PoolId graphicsPipelineId; 

	void defineLayouts();
	void defineResources(); 
	void definePasses(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	
	void setGPUSceneToTriangle();
}; 
}