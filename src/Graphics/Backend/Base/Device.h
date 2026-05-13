#pragma once 
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <set>
#include <cstring>
#include "Queues.h"
#include "Utilities.h"

// indicies (locations) of Queue Families (if they exist at all);
struct QueueFamilyIndicies {
	int32_t graphicsFamily = -1;	 // Location of Graphics Queue Family(includes Transfer capabilities by Vulkan Standard)
	int32_t presentationFamily = -1; // Location of Presentation Queue Family(not a real Queue Family, more of "Queue that supports Presentation")
	int32_t transferFamily = -1;
	int32_t computeFamily = -1;

	bool isValid() {
		return this->graphicsFamily >= 0 && this->presentationFamily >= 0 && this->transferFamily >= 0 && this->computeFamily >= 0;
	}
	// Todo: if there are no Transfer or Compute queues - assign their indices to available Graphics queue, so further code works anyways, just on one queue. 
};

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

