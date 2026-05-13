#pragma once 

#include <vector>
#include <vulkan/vulkan.h>
#include <string>

enum CmdBufferTypeEnum
{
	GRAPHICS,
	TRANSFER,
	COMPUTE
};


using Cmd = void(*)(VkCommandBuffer& rCmdBuffer_);

struct CommandBufferBlueprint {
	Cmd commandsToRecord;
	VkCommandBufferUsageFlags cmdBufferUsageFlags;

	bool exists = false; 

	void init(Cmd commandsToRecord_, VkCommandBufferUsageFlags cmdBufferUsageFlags_);
};
