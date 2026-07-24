#pragma once 
#include "Mesh.h"
#include "GraphicsPipeline.h"
#include "PoolId.h"
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>

namespace Graphics
{
struct TriangleVertexMemberIds 
{
	PoolId positionId; 
	PoolId colorId; 
}; 

struct Triangle { 
	PoolId vertexShaderId; 
	PoolId fragmentShaderId; 
	PoolId vertexLayoutId; 
	TriangleVertexMemberIds vertexMemberIds; 

	PoolId meshId;
	PoolId graphicsPipelineId; 

	void defineLayouts();
	void defineResources(); 
	void definePasses(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	
	Triangle();
}; 
}
