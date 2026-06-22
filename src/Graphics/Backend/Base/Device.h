#pragma once 
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <set>
#include <cstring>
#include "Queues.h"
#include "Utilities.h"

namespace Graphics
{
	struct Device {
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		QueueFamilyIndicies queueFamilyIndicies;
		Queues queues = {};
	
		void setup();
		void destroy();
		void getPhysicalDevice();
		void createLogicalDevice();
		bool checkDeviceSuitable(VkPhysicalDevice device_);
		bool checkDeviceExtensionsSupport(VkPhysicalDevice device_); // Just Swapchain for now.  
		QueueFamilyIndicies getQueueFamilies(VkPhysicalDevice& rDevice_);
	
	};
};
