#pragma once 

#include "Utilities.h"
#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include <functional>



using recordFunc = std::function<void(VkCommandBuffer&)>;

struct CommandBufferBlueprint {
	recordFunc commandsToRecord;
	VkCommandBufferUsageFlags cmdBufferUsageFlags;
	void init(recordFunc commandsToRecord_, VkCommandBufferUsageFlags cmdBufferUsageFlags_, bool oneTimeUse_);
};
