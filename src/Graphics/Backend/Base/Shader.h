#pragma once 
#include "Utilities.h"
#include <string>
#include <vector>

namespace Graphics
{
struct Shader {
	VkShaderModule vkHandle = VK_NULL_HANDLE;
	std::string path; 

	VkShaderModule get() const; 
	Shader(const std::string shaderPath_);
	~Shader();
	// move
	Shader(Shader&& other) noexcept; 
	Shader& operator=(Shader&& other) noexcept; 
	// delete copy
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
};
}
