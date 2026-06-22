#pragma once 
#include "Mesh.h"
#include "GraphicsPipeline.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>

namespace Graphics
{
struct Triangle { 
	Mesh mesh;
	uint32_t cmdBufferId;  

	void load(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	void submit(); // Triggers every frame
}; 
}
