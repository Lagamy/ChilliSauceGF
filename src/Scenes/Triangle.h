#pragma once 
#include "Mesh.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <array>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
struct Triangle { 
	Mesh mesh;
	
	void load(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	void submit(); // Triggers every frame
	Triangle(); 
}; 
}
