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
struct Triangle { 
	PoolId vertexShaderId; 
	PoolId fragmentShaderId; 
	PoolId verticeLayoutId; 
	Mesh mesh;

	uint32_t cmdBufferId;
	bool firstFrame = true;  

	void load(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	void submit(); // Triggers every frame
	Triangle();
}; 
}
