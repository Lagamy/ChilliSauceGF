// Exists to incapsulate every Descriptor sets / Push constants layout. So Graphics Pipeline knows - what it will work with.   
#pragma once 
#include <vulkan/vulkan.h>
#include <format>
#include "DescriptorSetLayout.h"

namespace Graphics
{
struct PipelineLayout {
	VkPipelineLayout vkHandle = VK_NULL_HANDLE;
	std::vector<DescriptorSetLayout> dsLayouts; 
	void create(); // Temporary hardcoded
	void destroy();
	
	VkPipelineLayout get() const;
};
}
