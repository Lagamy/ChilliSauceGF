#include "Shader.h"
#include "Api.h"
#include <stdexcept>

namespace Graphics
{

Shader::Shader(const std::string shaderPath_) 
{
	this->path = shaderPath_; 
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

Shader::~Shader() 
{
    this->destroy(); 
}

VkShaderModule Shader::get() const 
{
	return this->vkHandle;
}

Shader::Shader(Shader&& other) noexcept
{
    vkHandle = other.vkHandle;
    path = std::move(other.path);

    other.vkHandle = VK_NULL_HANDLE;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if(this != &other)
    {
        if(this->vkHandle != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(
                getMainDevice().logicalDevice,
                this->vkHandle,
                nullptr
            );
        }

        this->vkHandle = other.vkHandle;
        this->path = std::move(other.path);

        other.vkHandle = VK_NULL_HANDLE;
    }

    return *this;
}

void Shader::destroy()
{
    if(this->vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(getMainDevice().logicalDevice, this->vkHandle, nullptr);
        this->vkHandle = VK_NULL_HANDLE; 
    }   
}
}
