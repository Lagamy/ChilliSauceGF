#include "Shader.h"
#include "Api.h"

namespace Graphics
{


void Shader::create()
{
	const std::vector<char> code = Disk::readFile(this->path);
	// Build Shader Module
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkResult result = vkCreateShaderModule(getMainDevice().logicalDevice, &shaderModuleCreateInfo, nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Shader Module!");
	}
} 

void Shader::destroy()
{
	if(this->vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(getMainDevice().logicalDevice, this->vkHandle, nullptr);
	}
}

Shader::Shader(const std::string shaderPath_) 
{
	this->path = shaderPath_; 
}

Shader::~Shader() 
{
	this->destroy();
}

VkShaderModule Shader::get() const 
{
	return this->vkHandle;
}
}
