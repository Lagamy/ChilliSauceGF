#pragma once 
#include "Utilities.h"
#include <string>
#include <vector>

namespace Graphics
{
struct Shader {
	VkShaderModule vkHandle;
	std::string path; 

	VkShaderModule get() const; 
	Shader(const std::string shaderPath_);
	~Shader();
};
}
