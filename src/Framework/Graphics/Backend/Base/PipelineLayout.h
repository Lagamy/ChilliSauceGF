// Exists to incapsulate every Descriptor sets / Push constants layout. So Graphics Pipeline knows - what it will work with.   
#pragma once 
#include <vulkan/vulkan.h>
#include <format>
#include "DescriptorSetLayout.h"

namespace Graphics
{
struct PipelineLayout {
	VkPipelineLayout vkHandle = VK_NULL_HANDLE;
	
	std::vector<PoolId> usedByGraphicsPipelineIds; 
	std::vector<PoolId> usedByComputePipelineIds; 
	std::vector<PoolId> descriptorSetLayoutIds;

	void addDescriptorSetLayout(PoolId descriptorSetLayoutId_); // triggers recreate. 
	void create(); // creates pipelineLayout on GPU side.
	void recreate(); // Note: recreation is expensive(involves dsLayout vector reconstruction, traversal through derived Pipelines to recreate those too). Use it only inside of editors context. And avoid - if possible.  
	void destroy();
	
	VkPipelineLayout get() const;
};
}
