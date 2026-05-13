#pragma once 
#include "Utilities.h"
#include <string>
#include <vector>

struct Shader {
	VkShaderModule vkHandle;
	VkShaderModule get() const; 

	Shader(const std::string& shaderPath);
	~Shader();
};
