#pragma once 
#include "Utilities.h"
#include <string>
#include <vector>

namespace Graphics
{
/* descriptorId is the blueprint id of the descriptor, the same id setDescriptorMemberValue takes. */
struct ShaderDescriptorUse {
	uint32_t setIndex;
	PoolId descriptorId;
};

struct Shader {
	VkShaderModule vkHandle = VK_NULL_HANDLE;
	std::string path;
	std::vector<ShaderDescriptorUse> descriptors; // the descriptors the shader reads, declared by addDescriptor. Empty is fine. The declaration is what lets drawMeshIndexed check the descriptors and skip the draw until their first data is copied, so the shader has to declare every descriptor it reads, nothing checks the SPIR-V

	void addDescriptor(uint32_t setIndex_, PoolId blueprintId_);
	void destroy(); 
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
