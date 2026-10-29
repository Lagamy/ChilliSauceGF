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
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}
}

/*
	A pipeline layout copies the set layout handles when it is created, so adding a set layout to a created one rebuilds it.
*/
void PipelineLayout::addDescriptorSetLayout(PoolId descriptorSetLayoutId_)
{
	this->descriptorSetLayoutIds.emplace_back(descriptorSetLayoutId_);
	if(this->vkHandle != VK_NULL_HANDLE)
	{
		this->recreate();
	}
}

/*
	A pipeline keeps working after its layout object is destroyed, but a layout can't be destroyed while submitted commands still use it, and a pipeline can't be destroyed while commands that bind it are pending.
	Nothing here waits for the GPU: the caller has to make sure nothing submitted uses the layout or its pipelines (editor context only).
*/
void PipelineLayout::recreate()
{
	this->destroy();
	this->create();
	for(const PoolId& rPipelineId : this->usedByGraphicsPipelineIds)
	{
		GraphicsPipeline& rPipeline = getPipelinesManager().graphicsPipelines[rPipelineId];
		rPipeline.destroy();
		rPipeline.create();
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
