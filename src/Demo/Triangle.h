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

	inline uint64_t counter = 0; 

	void setupEnvironment(); 
	void defineLayouts();
	void defineResources(); 
	void definePasses(); 
	void updateResources(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	
	void setGPUSceneToTriangle();
}; 
}