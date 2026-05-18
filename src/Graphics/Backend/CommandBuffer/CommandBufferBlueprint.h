#pragma once 

#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include <functional>

enum CmdBufferTypeEnum
{
	GRAPHICS,
	TRANSFER,
	COMPUTE
};


using Cmd = std::function<void(VkCommandBuffer&)>;

struct CommandBufferBlueprint {
	Cmd commandsToRecord;
	VkCommandBufferUsageFlags cmdBufferUsageFlags;

	bool exists = false; 

	void init(Cmd commandsToRecord_, VkCommandBufferUsageFlags cmdBufferUsageFlags_);
};
