#include "PipelineLayout.h"
#include "Globals.h"

namespace Graphics
{
void PipelineLayout::create()
{
	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 0;
	layoutCreateInfo.pSetLayouts = nullptr;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(Demo::renderer.mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Loyaut for Graphics Pipeline for {}!");
	}
}

void PipelineLayout::destroy()
{
	if (vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(Demo::renderer.mainDevice.logicalDevice, vkHandle, nullptr);
		this->vkHandle = VK_NULL_HANDLE;
	}
}

VkPipelineLayout PipelineLayout::get() const
{
	return this->vkHandle;
}
}
