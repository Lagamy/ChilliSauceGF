#pragma once 
#include "Mesh.h"
#include "GraphicsPipeline.h"
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>

namespace Graphics
{
struct Triangle { 
	Shader vertexShader = Shader(Disk::executablePath + "/Assets/shaders/triangle/vert.spv");
	Shader fragmentShader = Shader(Disk::executablePath + "/Assets/shaders/triangle/frag.spv");
	Mesh mesh;
	uint32_t cmdBufferId;  

	void load(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	void submit(); // Triggers every frame
	Triangle();
	~Triangle();
}; 
}
