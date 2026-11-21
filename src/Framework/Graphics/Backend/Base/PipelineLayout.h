// Exists to incapsulate every Descriptor sets / Push constants layout. So Graphics Pipeline knows - what it will work with. Not created by the user: PipelinesManager derives it from the shaders of a pipeline and shares one between pipelines with the same set layouts.   
#pragma once 
#include <vulkan/vulkan.h>
#include <format>
#include "DescriptorSetLayout.h"
#include "PushConstantLayout.h"

namespace Graphics
{
struct PipelineLayout {
	VkPipelineLayout vkHandle = VK_NULL_HANDLE;
	
	std::vector<PoolId> usedByGraphicsPipelineIds; 
	std::vector<PoolId> usedByComputePipelineIds; 
	std::vector<PoolId> descriptorSetLayoutIds;
	std::vector<PushConstantUse> pushConstantUses; // sorted by stages, one range per use. The offsets are placed by PushConstantStorage in create()

	void validateLimits() const; // throws a readable error when the sets of this layout break a device limit, called by create()
	void create(); // creates pipelineLayout on GPU side.
	void recreate(); // Note: recreation is expensive(involves dsLayout vector reconstruction, traversal through derived Pipelines to recreate those too). Use it only inside of editors context. And avoid - if possible.  
	void destroy();
	
	VkPipelineLayout get() const;
	const PushConstantUse& getPushConstantUse(PoolId pushConstantLayoutId_) const; // throws if the layout is not one of its uses
};
}
