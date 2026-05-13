#include "Shader.h"
#include "Globals.h"

Shader::Shader(const std::string& shaderPath) 
{
	const std::vector<char> code = DiskUtilities::readFile(shaderPath);
	// Build Shader Module
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkResult result = vkCreateShaderModule(Demo::renderer.mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Shader Module!");
	}
}

Shader::~Shader() 
{
	vkDestroyShaderModule(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr);
}

VkShaderModule Shader::get() const 
{
	return this->vkHandle;
}
