#pragma once 
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <set>
#include <cstring>
#include "Utilities.h"

namespace Graphics
{
	struct SuitableDevice {
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties = {};
	};

	struct Device {
		std::vector<SuitableDevice> suitableDevices; // every physical device that passed checkDeviceSuitable
		uint32_t mainDeviceId = 0; // index into suitableDevices
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties = {}; // of physicalDevice
		VkDevice logicalDevice = VK_NULL_HANDLE;
		QueueFamilyIndices queueFamilyIndices;
		std::array<VkQueue, 4> queues = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};  
		
		void setup();
		void destroy();
		void getPhysicalDevice();
		void selectDevice(uint32_t id_);
		void createLogicalDevice();
		bool checkDeviceSuitable(VkPhysicalDevice device_);
		bool checkDeviceExtensionsSupport(VkPhysicalDevice device_); // Just Swapchain for now.  
		QueueFamilyIndices getQueueFamilies(VkPhysicalDevice& rDevice_);
	
	};
};
