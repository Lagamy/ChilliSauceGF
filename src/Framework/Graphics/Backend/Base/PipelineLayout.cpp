#include "PipelineLayout.h"
#include "Api.h"

namespace Graphics
{
void PipelineLayout::create()
{
	std::vector<VkDescriptorSetLayout> descriptorLayouts; 
	descriptorLayouts.reserve(this->descriptorSetLayoutIds.size());
	for(const PoolId& layoutId : this->descriptorSetLayoutIds)
	{
		descriptorLayouts.emplace_back(getDescriptorsManager().layouts[layoutId].get()); 
	}


	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = descriptorLayouts.size();
	layoutCreateInfo.pSetLayouts = descriptorLayouts.data();
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(getMainDevice().logicalDevice, &layoutCreateInfo, nullptr, &vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Layout {}!");
	}
}

void PipelineLayout::destroy()
{
	if (vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(getMainDevice().logicalDevice, vkHandle, nullptr);
		this->vkHandle = VK_NULL_HANDLE;
	}
}

VkPipelineLayout PipelineLayout::get() const
{
	return this->vkHandle;
}
}
